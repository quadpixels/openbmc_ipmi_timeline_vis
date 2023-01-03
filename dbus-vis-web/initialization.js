var g_dbus_pcap_module = undefined;
var g_main_gles_module = undefined;

function Init() {
    console.log('[Init] Initialization');
    ipmi_timeline_view.Canvas = document.getElementById('my_canvas_ipmi');
    dbus_timeline_view.Canvas = document.getElementById('my_canvas_dbus');
    boost_asio_handler_timeline_view.Canvas =
        document.getElementById('my_canvas_boost_asio_handler');
  
    // Hide all canvases until the user loads files
    ipmi_timeline_view.Canvas.style.display = 'none';
    dbus_timeline_view.Canvas.style.display = 'none';
    boost_asio_handler_timeline_view.Canvas.style.display = 'none';
  
    let v1 = ipmi_timeline_view;
    let v2 = dbus_timeline_view;
    let v3 = boost_asio_handler_timeline_view;
  
    // Link views
    v1.linked_views = [v2, v3];
    v2.linked_views = [v1, v3];
    v3.linked_views = [v1, v2];
  
    // Set accent color
    v1.AccentColor = 'rgba(0,224,224,0.5)';
    v2.AccentColor = 'rgba(0,128,0,  0.5)';
    v3.AccentColor = '#E22';

    DragElement(document.getElementById("highlighted_messages"));
    DragElement(document.getElementById("main_gles_div"));

    // Resolve this promise
    new DBusPCAPModule().then(m => {
      g_dbus_pcap_module = m;
      console.log("DBus PCAP Module Initialized.");
    });
}

function ReadFile(e) {
    console.log("ReadFile");
    var file = e.target.files[0];
    if (!file) {
        return;
    }
    console.log(file);
    
    var reader = new FileReader();
    reader.onload = function(e) {
        var contents = e.target.result;
        let buf = new Uint8Array(contents);

        console.log(">>> g_dbus_pcap_module.ccall")
        g_dbus_pcap_module.ccall("StartSendPCAPByteArray", "undefined", ["number"], buf.length);
        console.log("<<< g_dbus_pcap_module.ccall")

        const CHUNK_SIZE = 65536;
        const num_chunks = parseInt((buf.length - 1) / CHUNK_SIZE) + 1;
        for (let i=0; i<num_chunks; i++) {
            const offset = i*CHUNK_SIZE;
            const offset_end = Math.min(buf.length, (i+1) * CHUNK_SIZE);
            let chunk = buf.slice(offset, offset_end);
            ShowBlocker('Sending PCAP file ... ' + parseInt(offset / 1024) + "K / " +
            parseInt(buf.length / 1024) + "K");
            g_dbus_pcap_module.ccall("SendPCAPByteArrayChunk", "undefined", ["array", "number"],
            [chunk, chunk.length]);
        }

        ShowBlocker('Parsing PCAP file ...');

        g_ipmi_parsed_entries = [];

        RANGE_RIGHT_INIT = 0;
        RANGE_LEFT_INIT  = 1e20;
        g_dbus_pcap_module.ccall("EndSendPCAPByteArray", "undefined", [], [])
    };
    reader.readAsArrayBuffer(file);
}

var g_Blocker = document.getElementById('blocker');
var g_BlockerCaption = document.getElementById('blocker_caption');
function HideBlocker() {
  g_Blocker.style.display = 'none';
}
function ShowBlocker(msg) {
  g_Blocker.style.display = 'block';
  g_BlockerCaption.textContent = msg;
}

var g_Navigation = document.getElementById('div_navi_and_replay');
function ShowNavigation() {
  g_Navigation.style.display = 'block';
}

var g_WelcomeScreen = document.getElementById('welcome_screen');
function HideWelcomeScreen() {
  g_WelcomeScreen.style.display = 'none';
}

// UI elements
var g_group_by_dbus = document.getElementById('span_group_by_dbus');
var g_group_by_ipmi = document.getElementById('span_group_by_ipmi');
var g_group_by_asio =
    document.getElementById('span_group_by_boost_asio_handler')
var g_canvas_ipmi = document.getElementById('my_canvas_ipmi');
var g_canvas_dbus = document.getElementById('my_canvas_dbus');
var g_canvas_asio = document.getElementById('my_canvas_boost_asio_handler');

var g_dbus_pcap_status_content = document.getElementById('dbus_pcap_status_content');
var g_dbus_pcap_error_content = document.getElementById('dbus_pcap_error_content');
var g_btn_download_dbus_pcap = document.getElementById('btn_download_dbus_pcap');
var g_welcome_screen_content = document.getElementById('welcome_screen_content');
var g_scapy_error_content = document.getElementById('scapy_error_content');

var g_btn_zoom_reset = document.getElementById('btn_zoom_reset');
var g_cb_debug_info = document.getElementById('cb_debuginfo');

function ShowDBusTimeline() {
  g_canvas_dbus.style.display = 'block';
  g_group_by_dbus.style.display = 'block';
}
function ShowIPMITimeline() {
  g_canvas_ipmi.style.display = 'block';
  g_group_by_ipmi.style.display = 'block';
}
function ShowASIOTimeline() {
  g_canvas_asio.style.display = 'block';
  g_group_by_asio.style.display = 'block';
}

document.getElementById('file-input').addEventListener('change', ReadFile, false);

// Fancy Viz button
document.getElementById('btn_main_gles').addEventListener('click', function() {
  if (g_main_gles_module == undefined) {
    AddFancyVis();
  } else {
    RemoveFancyVis();
  }
});

document.getElementById('btn_start_replay').addEventListener('click', function() {
  StartReplay();
});

function UpdateFileNamesString() {} // No-op for browser version
var g_pcap_load_method = "transpiled";

g_renderer.run();
Init();