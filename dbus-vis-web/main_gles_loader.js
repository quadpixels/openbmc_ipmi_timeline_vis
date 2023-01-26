function AddFancyVis() {
  if (g_main_gles_module != undefined) return;
  document.getElementById("main_gles_div").style.display="block"
  new MainGLESModule({
    preRun: [],
    postRun: [],
    print: function() { 
        console.log(Array.prototype.slice.call(arguments).join(" "));
    },
    printErr: function() {
        console.warn(Array.prototype.slice.call(arguments).join(" "));
    },
    canvas: document.getElementById('main_gles_canvas')
  }).then(m => {
    g_main_gles_module = m;
    console.log("DBus PCAP Main GLES Module Initialized.");
  });
}

function RemoveFancyVis() {
  try {
    g_main_gles_module.abort();
  } catch (e) {

  }
  g_main_gles_module = undefined;
  document.getElementById("main_gles_div").style.display="none";
}

let g_is_replaying = false;
let g_replay_msg_idx = 0;
let g_replay_time = 0;
let g_replay_t0 = 0;
let g_replay_timeout;
let g_dbus_service_timerange = {};
let g_dbus_conn_visible = new Set();
function StartReplay() {
  StopReplay();
  g_replay_t0 = Date.now();
  g_replay_msg_idx = 0;
  g_is_replaying = true;

  g_dbus_conn_visible.clear();
  // Process time ranges
  g_dbus_service_timerange = {};
  for (let i=0; i<g_preproc.length; i++) {
    const msg = g_preproc[i];
    if (msg[0] == "mc") {
      const sender = msg[3], destination = msg[4];
      const out_time = msg[8];
      if (! (sender in g_dbus_service_timerange)) {
        g_dbus_service_timerange[sender] = [out_time, out_time];
      } else {
        g_dbus_service_timerange[sender][1] = Math.max(
          g_dbus_service_timerange[sender][1], out_time);
      }
      if (! (destination in g_dbus_service_timerange)) {
        g_dbus_service_timerange[destination] = [out_time, out_time];
      } else {
        g_dbus_service_timerange[destination][1] = Math.max(
          g_dbus_service_timerange[destination][1], out_time);
      }
    } else if (msg[0] == "sig") {
      const sender = msg[3];
      const timestamp = msg[8];
      if (! (sender in g_dbus_service_timerange)) {
        g_dbus_service_timerange[sender] = [timestamp, timestamp];
      } else {
        g_dbus_service_timerange[sender][1] = Math.max(
          g_dbus_service_timerange[sender][1], timestamp);
      }
    }
  }

  ReplayCallback();
}
function StopReplay() {
  if (g_replay_timeout != undefined) {
    clearTimeout(g_replay_timeout);
    g_replay_timeout = undefined;
  }
}
function ReplayCallback() {
  const fade_time = 2000;
  if (g_preproc.length < 1) return;
  const first_packet = g_preproc[0];
  const last_packet = g_preproc[g_preproc.length-1];
  if (g_main_gles_module == undefined) return;
  g_replay_time = Date.now();
  const replay_elapsed = g_replay_time - g_replay_t0;
  if (g_replay_msg_idx >= g_preproc.length &&
      replay_elapsed > fade_time * 2 + last_packet[1] - first_packet[1]) {
    g_is_replaying = false;
    console.log("Replay finished.");
    return;
  }

  if (!g_is_replaying) return;
  console.log(">> ReplayCallback");
  while (g_replay_msg_idx < g_preproc.length) {
    const msg = g_preproc[g_replay_msg_idx];
    const msg_millis = msg[1] - first_packet[1];
    if (msg_millis <= replay_elapsed) {
      if (msg[0] == "mc") {
        g_main_gles_module.ccall("DBusMakeMethodCall", "undefined", ["string","string","string","string","string"],
          [msg[3], msg[4], msg[5], msg[6], msg[7]]);
        g_dbus_conn_visible.add(msg[3]);
        g_dbus_conn_visible.add(msg[4]);
      } else if (msg[0] == "sig") {
        g_main_gles_module.ccall("DBusEmitSignal", "undefined", ["string", "string", "string", "string"],
        [msg[3], msg[4], msg[5], msg[6]]);
        g_dbus_conn_visible.add(msg[3]);
      }
      g_replay_msg_idx++;
    } else {
      const to_sleep = msg_millis - replay_elapsed;
      g_replay_timeout = setTimeout(ReplayCallback, to_sleep);
      break;
    }
  }

  // Process fade-out
  let a = Array.from(g_dbus_conn_visible);
  let victims = new Set();
  const ts = replay_elapsed + first_packet[1];
  for (let i=0; i<a.length; i++) {
    const conn = a[i];
    if (conn in g_dbus_service_timerange) {
      if (ts > 2000 + g_dbus_service_timerange[conn][1]) {
        victims.add(conn);
        g_main_gles_module.ccall("DBusServiceFadeOut", "undefined", ["string"], [conn]);
      }
    }
  }
  for (let v in Array.from(victims)) {
    delete g_dbus_conn_visible[v];
  }
}