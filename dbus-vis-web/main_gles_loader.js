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
function StartReplay() {
  StopReplay();
  g_replay_t0 = Date.now();
  g_replay_msg_idx = 0;
  g_is_replaying = true;
  ReplayCallback();
}
function StopReplay() {
  if (g_replay_timeout != undefined) {
    clearTimeout(g_replay_timeout);
    g_replay_timeout = undefined;
  }
}
function ReplayCallback() {
  if (g_preproc.length < 1) return;
  if (g_main_gles_module == undefined) return;
  if (g_replay_msg_idx >= g_preproc.length) {
    g_is_replaying = false;
    console.log("Replay finished.");
    return;
  }

  const first_packet = g_preproc[0];
  g_replay_time = Date.now();
  if (!g_is_replaying) return;
  console.log(">> ReplayCallback");
  while (g_replay_msg_idx < g_preproc.length) {
    const msg = g_preproc[g_replay_msg_idx];
    const replay_elapsed = g_replay_time - g_replay_t0;
    const msg_millis = msg[1] - first_packet[1];
    if (msg_millis <= replay_elapsed) {
      if (msg[0] == "mc") {
        g_main_gles_module.ccall("DBusMakeMethodCall", "undefined", ["string","string"],
          [msg[3], msg[4]]);
      }
      g_replay_msg_idx++;
    } else {
      const to_sleep = msg_millis - replay_elapsed;
      g_replay_timeout = setTimeout(ReplayCallback, to_sleep);
      break;
    }
  }
}