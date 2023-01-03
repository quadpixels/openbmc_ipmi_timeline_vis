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