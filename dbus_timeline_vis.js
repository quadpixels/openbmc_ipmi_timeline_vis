// Data and Timestamps are separate b/c dbus-pcap does not include
// timestamps in JSON output so we need to export both formats
// (JSON and text)
var Data_DBus       = []
var Timestamps_DBus = []
var StartingUsec_DBus = 0;

// Main view object
dbus_timeline_view = new TimelineView();
dbus_timeline_view.is_dbus = true;

// group-by condition changes
{
	const tags = ["dbus_column1", "dbus_column2", "dbus_column3",
	              "dbus_column4", "dbus_column5", "dbus_column6",
	              "dbus_column7"];
  for (let i=0; i<7; i++) {
		document.getElementById(tags[i]).addEventListener("click",
			OnGroupByConditionChanged_DBus);
	}
}

function draw_timeline_dbus(ctx) {
  dbus_timeline_view.Render(ctx);
}

let Canvas_DBus = document.getElementById("my_canvas_dbus");

function Preprocess_DBusText(data) {

}

function Group_DBus(preprocessed, group_by) {
  let grouped = { };
	const IDXES = { "Type":0, "Timestamp":1, "Serial":2, "Sender":3, "Destination":4, "Path":5, "Interface":6, "Member":7 }
	for (var n=0; n<preprocessed.length; n++) {
		var key = ""
		for (var i=0; i<group_by.length; i++) {
			if (i > 0) key += " "; key += ("" + preprocessed[n][IDXES[group_by[i]]]);
		}
		if (grouped[key] == undefined) grouped[key] = []
		grouped[key].push(preprocessed[n])
	}
	return grouped
}

function OnGroupByConditionChanged_DBus() {
	var tags = ["dbus_column1", "dbus_column2", "dbus_column3", "dbus_column4",
		          "dbus_column5", "dbus_column6", "dbus_column7" ];
	const v = dbus_timeline_view;
	v.GroupBy    = [];
	v.GroupByStr = "";
	for (let i=0; i<tags.length; i++) {
		let cb = document.getElementById(tags[i]);
		if (cb.checked) {
			v.GroupBy.push(cb.value);
			if (v.GroupByStr.length > 0) { v.GroupByStr += ", "; }
			v.GroupByStr += cb.value;
		}
	}
	let preproc = Preprocess_DBusPcap(Data_DBus, Timestamps_DBus); // should be from dbus_pcap
  let grouped = Group_DBus(preproc, v.GroupBy);
	GenerateTimeLine_DBus(grouped);
	dbus_timeline_view.IsCanvasDirty = true;
}

function GenerateTimeLine_DBus(grouped) {
  const keys = Object.keys(grouped);
	let sortedKeys = keys.slice();
	let intervals = [];
	let titles    = [];
	StartingUsec_DBus = undefined;
	for (let i=0; i<sortedKeys.length; i++) {
		titles.push(sortedKeys[i]);
		line = [];
    for (let j=0; j<grouped[sortedKeys[i]].length; j++) {
			let entry = grouped[sortedKeys[i]][j];
			let t0 = parseFloat(entry[1]) / 1000.0;
			let t1 = parseFloat(entry[8]) / 1000.0;

			// Assume IPMI dataset is loaded first
			if (StartingUsec_DBus == undefined) {
				StartingUsec_DBus = t0;
			}
			StartingUsec_DBus = Math.min(StartingUsec_DBus, t0);

			line.push([t0, t1, entry]);
		}
		intervals.push(line);
	}

  // Time shift
	for (let i=0; i<intervals.length; i++) {
		for (let j=0; j<intervals[i].length; j++) {
			let x = intervals[i][j];
			x[0] -= StartingUsec_DBus;
			x[1] -= StartingUsec_DBus;
		}
	}


	dbus_timeline_view.Intervals = intervals.slice();
	dbus_timeline_view.Titles    = titles.slice();
}

Canvas_DBus.onmousemove = function(event) {
	const v = dbus_timeline_view;
  v.MouseState.x = event.pageX - this.offsetLeft;
  v.MouseState.y = event.pageY - this.offsetTop;
  if (v.MouseState.pressed == true) { // Update highlighted area
    v.HighlightedRegion.t1 = v.MouseXToTimestamp(v.MouseState.x);
  }
  v.OnMouseMove();
  v.IsCanvasDirty = true;

	const u = ipmi_timeline_view;
  u.MouseState.x = event.pageX - this.offsetLeft;
  if (u.MouseState.pressed == true) { // Update highlighted area
    u.HighlightedRegion.t1 = u.MouseXToTimestamp(u.MouseState.x);
  }
  u.OnMouseMove();
  u.IsCanvasDirty = true;
}

Canvas_DBus.onmousedown = function(event) {
	if (event.button == 0) {
		dbus_timeline_view.OnMouseDown();
	}
};

Canvas_DBus.onmouseup = function(event) {
  if (event.button == 0) {
		dbus_timeline_view.OnMouseUp();
  }
}

Canvas_DBus.onwheel = function(event) {
	event.preventDefault();
	const v = dbus_timeline_view;
	if (v.IsMouseOverTimeline()) {
		let dz = 0;
		if (event.deltaY > 0) { // Scroll down, zoom out
			dz = -0.3;
		} else if (event.deltaY < 0) {  // Scroll up, zoom in
			dz = 0.3;
		}
		v.Zoom(dz,
			v.MouseXToTimestamp(
				v.MouseState.x));
	} else {
		if (event.deltaY > 0) { 
			v.ScrollY(1);
		} else if (event.deltaY < 0) { 
			v.ScrollY(-1);
		}
	}
}
