const { spawn } = require('child_process')
const targz = require('targz')

// Capture state for all scripts
var g_capture_state = "not started"
var g_capture_mode  = "live"

function currTimestamp() {
	var tmp = new Date()
	return (tmp.getTime() + tmp.getTimezoneOffset() * 60000) / 1000
}

var g_child
var g_rz

var g_capture_live = true

function streamWrite(stream, chunk, encoding='utf8') {
	return new Promise((resolve, reject) => {
		const errListener = (err) => {
			stream.removeListener('error', errListener);
			reject(err);
		};
		stream.addListener('error', errListener);
		const callback = () => {
			stream.removeListener('error', errListener);
			resolve(undefined);
		};
		stream.write(chunk, encoding, callback);
	});
}

function ExtractTarFile() {
	const tar_file = "DBUS_MONITOR.tar.gz"
	const target   = "."

  targz.decompress({
		src: tar_file,
		dest: target
	}, function(err) {
		if (err) {
			console.log("Error decompressing .tar.gz file:" + err)
		} else {
			console.log("Done! will load file contents")
			fs.readFile("./DBUS_MONITOR", { encoding:"utf-8" },
				(err,data) => {
					if (err) { console.log("Error in readFile: " + err) }
					else {
						ParseIPMIDump(data)
					}
				}
			);
		}
	});
}

async function OnTransferCompleted() {
  g_rz.kill("SIGINT")
	g_child.kill("SIGINT")

	capture_info.textContent = "Loaded the capture file"
	OnCaptureStop()
	ExtractTarFile()
}

async function LaunchRZ() {
  // On the Host
	g_rz = spawn("rz", ["-vv", "-b", "-E"], {shell:true})
  g_rz.stdout.on("data", (data)=>{
		console.log("[rz] received " + data.length + " B")
		g_child.stdin.write(data)
	});
	g_rz.stderr.on("data", (data)=>{
		console.log("[rz] error: " + data)
		if (data.indexOf("Transfer complete") != -1) {
			OnTransferCompleted()
		}
	});
	await Promise.all([
		g_rz.stdin.pipe(g_child.stdout),
		g_rz.stdout.pipe(g_child.stdin)
	])
}

function ClearAllPendingTimeouts() {
	var id = setTimeout(function(){}, 0);
	for (; id >= 0; id--) clearTimeout(id)
}

function QueueDbusMonitorFileSize(secs = 5) {
	setTimeout(function() {
		g_child.stdin.write("a=`ls -l DBUS_MONITOR | awk '{print $5}'` ; echo \">>>>>>$a<<<<<<\"  \n\n\n\n")
	}, secs*1000)
}

function StopCapture() {
	switch (g_capture_mode) {
		case "live":
			g_child.stdin.write("\x03 ")
			g_capture_state = "stopping"
			capture_info.textContent = "Ctrl+C sent to BMC console"
			break;
		case "staged":
			g_child.stdin.write("echo \">>>>>>\" && killall dbus-monitor && echo \"<<<<<<\" \n\n\n\n")
//			g_child.stdin.write("\x03 ")
			g_capture_state = "stopping"
			capture_info.textContent = "Stopping dbus-monitor"
			break;
	}
}

async function StartCapture(host) {

	// Disable buttons
	btn_start_capture.disabled  = true
	select_capture_mode.disabled = true

	// On the B.M.C.
	let last_t = currTimestamp()
	let attempt = 0
//  const ls = spawn("bash", ["/tmp/DELAYS.bash"]);
//  g_child = spawn("ssh", ["localhost"], {shell:true});
	g_child = spawn("megapede_client", [host], {shell:true})
  g_child.stdout.on('data', async function(data) {
		var t = currTimestamp()
		console.log("[bmc] " + data)
		{
			switch (g_capture_state) {
				case "not started": 
					attempt ++
					console.log("attempt " + attempt)
 					g_child.stdin.write("echo \"haha\" \n")
//					await streamWrite(g_child.stdin, "whoami \n")
					let idx = data.indexOf("haha")
					if (idx != -1) {
						OnCaptureStart() // Successfully logged on, start
						g_capture_state = "dbus monitor start"
					} else {
            console.log("idx=" + idx)
					}
					break
				case "dbus monitor start":
					if (g_capture_mode == "live") {
						// Make sure console bit rate is greater than
						//   the speed outputs are generated!
						g_child.stdin.write("dbus-monitor --system | grep \"sendMessage\\|ReceivedMessage\" -A7 \n")
					} else {
						g_child.stdin.write("dbus-monitor --system | grep \"sendMessage\\|ReceivedMessage\" -A7 > /run/initramfs/DBUS_MONITOR & \n\n\n")
						/*
						setTimeout(function() {
							g_child.stdin.write("\x03  ")
							setTimeout(function() {
								g_capture_state = "dbus monitor end"
								g_child.stdin.write("echo \">>>>\"; ls -l  /run/initramfs/HAHA | awk '{print $5}' ; echo \"<<<<\" \n\n\n\n")
							}, 1000) // Give BMC 2s to react
						}, 10000)
						*/
						QueueDbusMonitorFileSize(2)
					}
					g_capture_state = "dbus monitor running"
					break
				case "dbus monitor running":
					if (g_capture_mode == "staged") {
						let s = data.toString()
						let i0 = s.lastIndexOf(">>>>>>"), i1 = s.lastIndexOf("<<<<<<")
						if (i0 != -1 && i1 != -1) {
              let tmp = s.substr(i0+6, i1-i0-6)
							let sz = parseInt(tmp) / 1024
							capture_info.textContent = "Uncompressed dbus capture size: " + sz + " KiB"
							QueueDbusMonitorFileSize(5)
						}
					}
					console.log("monitor result updated")
					AppendToParseBuffer(data.toString())
					MunchLines()
					UpdateLayout()
					break
				case "dbus monitor end":
					let s = data.toString()
					let i0 = s.lastIndexOf(">>>>"), i1 = s.lastIndexOf("<<<<")
					if (i0 != -1 && i1 != -1) {
						let tmp = s.substr(i0+4, i1-i0-4)
						let sz = parseInt(tmp)
						if (isNaN(sz)) {
							console.log("Error: the tentative dbus-profile dump is not found!")
						} else {
							let bps = sz / 10
							console.log("dbus-monitor generates " + bps + "B per second")
						}
					}
					g_child.kill("SIGINT")
					break
				case "sz sending":
					console.log("Received a chunk of size " + data.length)
					capture_info.textContent = "Received a chunk of size " + data.length
					g_rz.stdin.write(data)
					break
				case "stopping":
					let t = data.toString();
					if (g_capture_mode == "live") {
						if (t.lastIndexOf("^C") != -1) {
							// Live mode
							g_child.kill("SIGINT")
							g_capture_state = "not started"
							OnCaptureStop()
							capture_info.textContent = "connection to BMC closed"
							// Log mode
						}
					} else if (g_capture_mode == "staged") {
						
            ClearAllPendingTimeouts()

						if (t.lastIndexOf("<<<<<<") != -1) {
							g_capture_state = "compressing"
							g_child.stdin.write("echo \">>>>>>\" && cd /run/initramfs && tar cfz DBUS_MONITOR.tar.gz DBUS_MONITOR && echo \"<<<<<<\" \n\n\n\n")
							capture_info.textContent = "Compressing dbus monitor dump on BMC"
						}
					}
					break
				case "compressing":
					g_child.stdin.write("echo \">>>>>>\" && ls -l HOHOHO | awk '{print $5}' && echo \"<<<<<<\"   \n\n\n\n")
					g_capture_state = "dbus_monitor size"
					capture_info.textContent = "Obtaining size of compressed dbus dump"
		      break
				case "dbus_monitor size":
          // Starting RZ
				  g_capture_state = "sz start"
					capture_info.textContent = "Starting rz and sz"
					g_child.stdin.write("sz -w 65536 -y /run/initramfs/DBUS_MONITOR.tar.gz\n")
//				g_child.stdin.write("sz -w 65536 -y /tmp/haha\n")
					g_capture_state = "sz sending"
					LaunchRZ()
					break
			}
			last_t = t
	  }
	});
  g_child.stderr.on('data', (data)=>{
		console.log("[bmc] err=" + data);
		g_child.stdin.write("echo \"haha\" \n\n")
	});
	g_child.on("close", (code)=>{
		console.log("return code: " + code)
	});
}

