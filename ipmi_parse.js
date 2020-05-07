// This file parses ASCII text-encoded dbus message dump.

function extractUsec(line) {
  let i0 = line.indexOf("time=")
	if (i0 == -1) { return BigInt(-1) }
	let line1 = line.substr(i0)
	let i1 = line1.indexOf(" ")
	if (i1 == -1) { return BigInt(-1) }
	let line2 = line1.substr(5, i1-5)
	let sp = line2.split(".")
	return BigInt(sp[0]) * BigInt(1000000) +
	       BigInt(sp[1])
}

// Returns [byte, i+1] if successful
// Returns [null, i  ] if unsuccessful
function munchByte(lines, i) {
  let l = lines[i]
  let idx = l.indexOf("byte")
  if (idx != -1) {
    return [parseInt(l.substr(idx+4), 10), i+1]
  } else {
    return [null, i]
  }
}

// array of bytes "@"
function munchArrayOfBytes1(lines, i) {
  let l = lines[i]
  let idx = l.indexOf("array of bytes \"")
  if (idx != -1) {
    let the_ch = l.substr(idx+16, 1)
    return [[the_ch.charCodeAt(0)], i+1]
  } else {
    return [null, i]
  }
}

function munchArrayOfBytes2(lines, i) {
  let l = lines[i]
  let idx = l.indexOf("array of bytes [")
  if (idx != -1) {
    let j = i+1
    let payload = []
    while (true) {
      l = lines[j]
      let sp = l.trim().split(" ")
      let ok = true
      for (let k = 0; k < sp.length; k++) {
        let b = parseInt(sp[k], 16)
        if (isNaN(b)) { ok = false; break }
        else { payload.push(b) }
      }
      if (!ok) {
        j--; break;
      }
      else j++;
    }
    return [payload, j]
  } else {
    return [null, i]
  }
}

function munchArrayOfBytes(lines, i) {
  let x = munchArrayOfBytes1(lines, i)
	if (x[0] != null) { return x }
	x = munchArrayOfBytes2(lines, i)
  if (x[0] != null) { return x }
	return [ null, i ]
}

// ReceivedMessage
function munchLegacyMessageStart(lines, i) {
  let entry = { netfn: 0, lun: 0, cmd: 0, serial: 0,
	              start_usec: 0, end_usec: 0,
								request: [], response: [] }

  let ts = extractUsec(lines[i])
	entry.start_usec = ts
	
	let x = munchByte(lines, i+1)
	if (x[0] == null) { return [null, i] }
	entry.serial = x[0]
	let j = x[1]

	x = munchByte(lines, j)
	if (x[0] == null) { return [null, i] }
  entry.netfn  = x[0]; j = x[1]

	x = munchByte(lines, j)
	if (x[0] == null) { return [null, i] }
	entry.lun    = x[0]; j = x[1]

	x = munchByte(lines, j)
	if (x[0] == null) { return [null, i] }
	entry.cmd    = x[0]; j = x[1]

	x = munchArrayOfBytes(lines, j)
	if (x[0] == null) { return [null, i] }
	entry.request = x[0]; j = x[1]

	return [entry, j]
}

function munchLegacyMessageEnd(lines, i, in_flight, parsed_entries) {
  let ts = extractUsec(lines[i])

  let x = munchByte(lines, i+1)
	if (x[0] == null) { return [null, i] } // serial
	let serial = x[0]; let j = x[1]

  let entry = null
	if (serial in in_flight) { 
      entry = in_flight[serial]
      delete in_flight[serial]
    }
  else { return [null, i] }

	entry.end_usec = ts

	x = munchByte(lines, j) // netfn
	if (x[0] == null) { return [null, i] }
  if (entry.netfn + 1 == x[0]) { } else { return [null, i] }
	j = x[1]

	x = munchByte(lines, j) // lun (not used right now)
	if (x[0] == null) { return [null, i] }
	j = x[1]

	x = munchByte(lines, j) // cmd
	if (x[0] == null) { return [null, i] }
	if (entry.cmd == x[0]) { } else { return [null, i] }
	j = x[1]

  x = munchByte(lines, j) // cc
	if (x[0] == null) { return [null, i] }
	j = x[1]

	x = munchArrayOfBytes(lines, j)
	if (x[0] == null) { return [null, i] }
	else { entry.response = x[0] }
	j = x[1]

	parsed_entries.push(entry)

	return [ entry, j ]
}

function ParseIPMIDump(data) {
  console.log("data.length=" + data.length)
  lines = data.split("\n")
  console.log("" + lines.length + " lines")
    
  let in_flight_legacy = { }
	let parsed_entries   = []
  let i = 0
  while (i<lines.length) {
    let line = lines[i]
    if (line.indexOf("interface=org.openbmc.HostIpmi") != -1 &&
        line.indexOf("member=ReceivedMessage") != -1) {
			let x = munchLegacyMessageStart(lines, i)
			let entry = x[0]; i = x[1];
			if (entry != null) { in_flight_legacy[entry.serial] = entry }
    } else if (line.indexOf("interface=org.openbmc.HostIpmi") != -1 &&
               line.indexOf("member=sendMessage") != -1) {
			let x = munchLegacyMessageEnd(lines, i, in_flight_legacy, parsed_entries);

    } else if (line.indexOf("interface=xyz.openbmc_project.Ipmi.Server") != -1 &&
               line.indexOf("member=execute") != -1) {
      console.log("2 start")
    } else if (line.indexOf("method return") != -1) {
      console.log("maybe 2 end")
    }
    i++
  }

	console.log("Number of parsed entries: " + parsed_entries.length)

  if (parsed_entries.length > 0) {
  	// Write to Data
		let ts0 = parsed_entries[0].start_usec
		let ts1 = parsed_entries[parsed_entries.length-1].end_usec
  	Data = []
  	for (i=0; i<parsed_entries.length; i++) {
  	  let entry = parsed_entries[i]
  		let x = [ entry.netfn, entry.cmd, 
			  parseInt(entry.start_usec - ts0),
				parseInt(entry.end_usec   - ts0),
				entry.request,
				entry.response
			]
			Data.push(x)
  	}

		// Re-calculate time range
		RANGE_LEFT_INIT  = 0
		RANGE_RIGHT_INIT = parseInt((ts1-ts0) / BigInt(1000000) / BigInt(10)) * 10 + 10

		// Perform layout again
		IsCanvasDirty = true
		OnGroupByConditionChanged()
    BeginSetBoundaryAnimation(RANGE_LEFT_INIT, RANGE_RIGHT_INIT)
		ComputeHistogram()
	} else {
	  console.log("No entries parsed")
	}
}


// Load dummy data
function LoadDummyData() {
	document.getElementById("file_name").textContent = "(demo file)"
	Data = DummyData.slice()
	IsCanvasDirty = true
	OnGroupByConditionChanged()
	RANGE_RIGHT_INIT = 60
	BeginSetBoundaryAnimation(RANGE_LEFT_INIT, RANGE_RIGHT_INIT)
	ComputeHistogram()
}

// Load dummy data upon starting of the program
LoadDummyData()
