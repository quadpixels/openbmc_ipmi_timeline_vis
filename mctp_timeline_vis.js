// This file deals with MCTP messages.

// Fields: [src_eid, dest_eid, t0, t1, desc, frag_count]

const dummy_mctp_msgs = [
  [ 10, 16, 1,   2,   "No desc", 1 ],
  [ 10, 16, 2.1, 3.1, "No desc", 2 ],
]

{
  const tags = [
    'mctp1', 'mctp2', 'mctp3'
  ];
  for (let i = 0; i < tags.length; i++) {
    document.getElementById(tags[i]).addEventListener(
        'click', OnGroupByConditionChanged_MCTP);
  }
}

var MCTP_Data = [];
var MCTP_Timestamp = [];

function Group_MCTP(preprocessed, group_by) {
  let grouped = {};
  const IDXES = {
    'Source EID': 0,
    'Destination EID': 1,
    'Description': 4
  };
  for (let n=0; n<preprocessed.length; n++) {
    let key = '';
    for (let i=0; i<group_by.length; i++) {
      if (i > 0) key += ' ';
      key += ('' + preprocessed[n][IDXES[group_by[i]]]);
    }
    if (grouped[key] == undefined) grouped[key] = [];
    grouped[key].push(preprocessed[n]);
  }
  return grouped;
}

// Layout Processing

function OnGroupByConditionChanged_MCTP() {
  var tags = ['mctp1', 'mctp2', 'mctp3'];
  const v = mctp_timeline_view;
  v.GroupBy = [];
  v.GroupByStr = '';
  for (let i=0; i<tags.length; i++) {
    let cb = document.getElementById(tags[i]);
    if (cb.checked) {
      v.GroupBy.push(cb.value);
      if (v.GroupByStr.length > 0) {
        v.GroupByStr += ', ';
      }
      v.GroupByStr += cb.value;
    }
  }
  let preproc = MCTP_Data;
  let grouped = Group_MCTP(preproc, v.GroupBy);
  GenerateTimeLine_MCTP(grouped);
  mctp_timeline_view.IsCanvasDirty = true;
}

function GenerateTimeLine_MCTP(grouped) {
  const keys = Object.keys(grouped)
  let sorted_keys = keys.slice()
  let intervals = [];
  let titles = [];


  if (g_StartingSec == undefined && MCTP_Data.length > 0) {
    g_StartingSec = MCTP_Data[0][2];
  }

  for (let i=0; i<sorted_keys.length; i++) {
    titles.push({"header": false, "title": sorted_keys[i],
      "intervals_idxes":[i]});
    line = [];
    for (let j=0; j<grouped[sorted_keys[i]].length; j++) {
      let entry = grouped[sorted_keys[i]][j];
      let t0 = parseFloat(entry[2]) - g_StartingSec;
      let t1 = parseFloat(entry[3]) - g_StartingSec;
      line.push([t0, t1, entry, 'ok', 0]);

      RANGE_RIGHT_INIT = Math.max(RANGE_RIGHT_INIT,
        (isNaN(t1) ? t0 : t1));
      RANGE_LEFT_INIT  = Math.min(RANGE_LEFT_INIT,  t0);
    }
    intervals.push(line);
  }

  mctp_timeline_view.Intervals = intervals.slice();
  mctp_timeline_view.Titles = titles.slice();
  mctp_timeline_view.LayoutForOverlappingIntervals();
}

// UI event handling

mctp_timeline_view = new MCTPTimelineView();

function draw_timeline_mctp(ctx) {
  mctp_timeline_view.Render(ctx);
}

let Canvas_Mctp = document.getElementById('my_canvas_mctp');

Canvas_Mctp.onmousemove = function(event) {
  const v = mctp_timeline_view;
  v.MouseState.x = event.pageX - this.offsetLeft;
  v.MouseState.y = event.pageY - this.offsetTop;
  if (v.MouseState.pressed == true &&
      v.MouseState.hoveredSide == 'timeline') {  // Update highlighted area
    v.HighlightedRegion.t1 = v.MouseXToTimestamp(v.MouseState.x);
  }
  v.OnMouseMove();
  v.IsCanvasDirty = true;

  v.linked_views.forEach(function(u) {
    u.MouseState.x = event.pageX - Canvas_Mctp.offsetLeft;
    u.MouseState.y = 0;                  // Do not highlight any entry
    if (u.MouseState.pressed == true &&
        u.MouseState.hoveredSide == 'timeline') {  // Update highlighted area
      u.HighlightedRegion.t1 = u.MouseXToTimestamp(u.MouseState.x);
    }
    u.OnMouseMove();
    u.IsCanvasDirty = true;
  });
};

Canvas_Mctp.onmousedown = function(event) {
  if (event.button == 0) {
    mctp_timeline_view.OnMouseDown();
  }
};

Canvas_Mctp.onmouseup = function(event) {
  if (event.button == 0) {
    mctp_timeline_view.OnMouseUp();
  }
};

Canvas_Mctp.onwheel = function(event) {
  mctp_timeline_view.OnMouseWheel(event);
}

function PopulateDummyMCTPTimeline() {
  MCTP_Data = dummy_mctp_msgs.slice();
  for (let x=4; x<12; x+=2) {
    OnNewMCTPRequestAndResponse(10, 16, x, x+0.2, 5, "No desc");
    OnNewMCTPRequestAndResponse(11, 16, x, x+0.2, 5, "No desc");
    OnNewMCTPUnmatchedRequest(12, 16, x, "No desc");
  }
  OnGroupByConditionChanged_MCTP();
}