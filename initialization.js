document.getElementById("bah1").addEventListener("click", OnGroupByConditionChanged_ASIO);
document.getElementById("bah2").addEventListener("click", OnGroupByConditionChanged_ASIO);

function Init() {
	console.log("[Init] Initialization");
	ipmi_timeline_view.Canvas = document.getElementById("my_canvas_ipmi");
	dbus_timeline_view.Canvas = document.getElementById("my_canvas_dbus");
	boost_asio_handler_timeline_view.Canvas = document.getElementById("my_canvas_boost_asio_handler");

  let v0 = ipmi_timeline_view;
	let v1 = dbus_timeline_view;
	let v2 = boost_asio_handler_timeline_view;

	// Link views
	v0.linked_views = [v1, v2];
	v1.linked_views = [v0, v2];
	v2.linked_views = [v0, v1];

	// Set accent color
	v0.AccentColor = "#0CC";
	v1.AccentColor = "#080";
	v2.AccentColor = "#E22";

	// Initialization stuff all go here
	OnGroupByConditionChanged();
	ComputeHistogram();

	// The following is for loading dummy IPMI data & is deprecated
	// when moving to DBus
	// Load dummy data upon starting of the program
	// LoadDummyData()
}

var g_Blocker = document.getElementById("blocker");
var g_BlockerCaption = document.getElementById("blocker_caption");
function HideBlocker() { g_Blocker.style.display = "none"; }
function ShowBlocker(msg) {
	g_Blocker.style.display = "block";
	g_BlockerCaption.textContent = msg;
}

//TestFileRead_BoostHandler()
Init();
