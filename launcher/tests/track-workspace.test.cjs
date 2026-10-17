const {test}=require('node:test'),assert=require('node:assert/strict');
test('track workspace identifies source, supports point selection and distinguishes driveable route edits from visual geometry',async()=>{
  const {trackWorkspace}=await import('../renderer/tracks.mjs');
  const track={name:'Coast <test>',bank:88,grade:1,available:true,blocks:[]};
  const point={x:100,y:-20,z:-100,angle:123,pitch:10,crossSlope:-2,leftWidth:50,rightWidth:80,length:200,arc:0};
  const html=trackWorkspace([track],88,{road:{points:[point,{...point,x:500,z:200}],arcs:[],length:400},blocks:[]},1);
  assert.match(html,/Coast &lt;test&gt;/);
  assert.match(html,/data-track-point="0"/);assert.match(html,/data-track-point="1"/);
  assert.match(html,/Road point 2/);assert.match(html,/not a terrain/);
  assert.match(html,/not the visible asphalt mesh/);assert.match(html,/Save point to project/);assert.ok(!html.includes('NaN'));
  const degenerate=trackWorkspace([track],88,{road:{points:[point],arcs:[]}},0);
  assert.ok(!degenerate.includes('NaN'));assert.ok(!degenerate.includes('Infinity'));
  for(const tab of ['road','scenery','atmosphere','race','all'])assert.match(html,new RegExp(`data-track-tab="${tab}"`));
  assert.match(html,/data-track-stage="2d"/);assert.match(html,/data-track-stage="3d"/);
  assert.match(html,/id="track-inline-preview"/);
});
test('track object form separates transforms from advanced flags and displays degrees',async()=>{
 const {trackWorkspace}=await import('../renderer/tracks.mjs');
 const scene={revision:7,objects:[{x:12,y:-30,z:45,yaw:1024,model:3,flags:4},{x:0,y:0,z:0,yaw:0,model:-1,flags:0}]};
 const html=trackWorkspace([{name:'Coast',bank:88,grade:1}],88,scene,0,0);
 assert.match(html,/Object 2 · disabled/);assert.match(html,/data-revision="7"/);
 assert.match(html,/id="object-yaw"[^>]*value="90"/);assert.match(html,/Rotation \(degrees\)/);
 const form=html.slice(html.indexOf('<form id="track-object-form"'));
 const [basic,advanced]=form.split('<details');
 for(const field of ['x','y','z','yaw'])assert.ok(basic.includes(`data-object-field="${field}"`));
 for(const field of ['model','flags'])assert.ok(!basic.includes(`data-object-field="${field}"`)&&advanced.includes(`data-object-field="${field}"`));
 assert.match(html,/Save object to project/);assert.match(html,/Save & apply project/);
});
test('road form exposes every point field with matching original values and advanced units',async()=>{
 const {trackWorkspace}=await import('../renderer/tracks.mjs');
 const html=trackWorkspace([{name:'Coast',bank:88}],88,{revision:4,road:{points:[{x:100,z:200,y:-20,angle:123,pitch:10,crossSlope:-2,leftWidth:50,rightWidth:80,length:200,arc:0}],arcs:[]}});
 const form=html.slice(html.indexOf('<form id="road-point-form"'),html.indexOf('</form>')+7),[basic,advanced]=form.split('<details');
 for(const f of ['x','y','z','left_width','right_width'])assert.ok(basic.includes(`data-road-field="${f}"`));
 for(const f of ['angle','pitch','cross_slope','length','arc'])assert.ok(advanced.includes(`data-road-field="${f}"`));
 assert.match(form,/id="road-left_width"[^>]*value="50" data-original="50"/);
 assert.match(form,/id="road-cross_slope"[^>]*value="-2" data-original="-2"/);
 assert.match(form,/data-revision="4"/);assert.ok(!form.includes('undefined'));
});
test('curved point exposes its shared arc center with editable coordinates',async()=>{
 const {trackWorkspace}=await import('../renderer/tracks.mjs');
 const point={x:100,z:200,y:0,angle:0,pitch:0,crossSlope:0,leftWidth:50,rightWidth:50,length:200,arc:1};
 const html=trackWorkspace([{name:'Coast',bank:88}],88,{revision:7,road:{points:[point],arcs:[{x:500,z:-600,reserved:123}]}});
 assert.match(html,/id="road-arc-form"[^>]*data-index="0"[^>]*data-revision="7"/);
 assert.match(html,/id="arc-x"[^>]*value="500" data-original="500"/);
 assert.match(html,/id="arc-z"[^>]*value="-600" data-original="-600"/);
 assert.match(html,/Shared by every segment/);assert.ok(!html.includes('data-arc-field="reserved"'));
});
test('road overview samples shortest arcs, mirrored modes, wraparound and changed centers',async()=>{
 const {roadPolyline}=await import('../renderer/tracks.mjs');
 const road={points:[{x:10,z:0,arc:1},{x:0,z:10,arc:0}],arcs:[{x:0,z:0}]};
 const line=roadPolyline(road);assert.equal(line.length,18);assert.deepEqual(line[0],{x:10,z:0});assert.deepEqual(line.at(-1),line[0]);
 for(const p of line.slice(0,16))assert.ok(Math.abs(Math.hypot(p.x,p.z)-10)<1e-8);
 assert.ok(line[8].x>7&&line[8].z>7);assert.deepEqual(line[16],{x:0,z:10});
 const mirrored=structuredClone(road);mirrored.points[0].arc=2;assert.deepEqual(roadPolyline(mirrored),line);
 const reversed={...road,points:[{x:0,z:10,arc:1},{x:10,z:0,arc:0}]};assert.ok(roadPolyline(reversed)[8].x>7&&roadPolyline(reversed)[8].z>7);
 const wrap={points:[{x:-10,z:1,arc:1},{x:-10,z:-1,arc:0}],arcs:[{x:0,z:0}]};assert.ok(roadPolyline(wrap).every(p=>p.x<0));
 const changed=structuredClone(road);changed.arcs[0].x=1;assert.notDeepEqual(roadPolyline(changed),line);
 const missing=structuredClone(road);missing.arcs=[];assert.deepEqual(roadPolyline(missing),[{x:10,z:0},{x:0,z:10},{x:10,z:0}]);
 assert.deepEqual(roadPolyline({points:[]}),[]);
});
test('native track bank actions use selected course/class, revision and explicit renderer limits',async()=>{
 const {trackWorkspace}=await import('../renderer/tracks.mjs');
 const html=trackWorkspace([{name:'Oval',bank:134}],134,{revision:12});
 for(const name of ['course','terrain']){
  assert.ok(html.includes(`data-export-track-mesh="track.oval6.${name}"`));
  assert.match(html,new RegExp(`data-import-track-mesh="track.oval6.${name}" data-revision="12"`));
 }
 assert.match(html,/Advanced · Native model banks/);assert.match(html,/modern renderer/);assert.match(html,/GLB with embedded textures/);
});
test('placed object GLB and OBJ actions address shared source model and remain separate from native banks',async()=>{
 const {trackWorkspace}=await import('../renderer/tracks.mjs');
 const html=trackWorkspace([{name:'Coast',bank:88}],88,{revision:5,objects:[{model:7,x:0,y:0,z:0,yaw:0,flags:0}]});
 assert.match(html,/data-preview-track-part="track.big1.course" data-part="7"/);
 assert.match(html,/data-export-track-part="track.big1.course" data-part="7" data-format="glb"/);assert.match(html,/data-export-track-part="track.big1.course" data-part="7" data-format="obj"/);
 assert.match(html,/data-import-track-part="track.big1.course" data-part="7" data-revision="5"/);
 assert.match(html,/shared by every object/);assert.match(html,/repeated exports add a number/);assert.match(html,/OBJ export includes PNG textures/);
});
test('terrain cell editor addresses empty cells and separates assignments from shared model edits',async()=>{
 const {terrainCellEditor}=await import('../renderer/tracks.mjs'),cells=Array(1024).fill(1023);cells[259]=0x7c01;
 const scene={revision:8,terrain:{models:3,cells}},html=terrainCellEditor(scene,88,259);
 assert.match(html,/data-cell="259"/);assert.match(html,/id="terrain-cell-form"[^>]*data-index="259"[^>]*data-revision="8"/);
 assert.match(html,/data-terrain-model data-original="1"/);assert.match(html,/data-preview-track-part="track.big1.terrain"[^>]*data-part="1"/);
 assert.match(html,/X 7168, Z 48128/);assert.match(html,/Visibility settings are preserved/);assert.match(html,/repeated exports add a number/);
 assert.match(html,/class="terrain-grid-map"/);assert.match(html,/data-select-terrain-cell="259"/);
 const empty=terrainCellEditor(scene,88,0);assert.match(empty,/value="1023" selected>Empty cell/);assert.match(empty,/data-terrain-model-action[^>]*hidden/);
 assert.equal(terrainCellEditor({},88),'');assert.match(terrainCellEditor(scene,88,2000),/data-cell="0"/);
});
test('scenery controls choose course models, address stable object IDs and also allow an empty list',async()=>{
 const {trackWorkspace}=await import('../renderer/tracks.mjs'),track={name:'Coast',bank:88},point={x:0,y:0,z:0,angle:0,pitch:0,crossSlope:0,leftWidth:100,rightWidth:100,length:100,arc:0};
 const scene={revision:9,courseModels:2,road:{points:[point],arcs:[]},objects:[{model:1,x:0,y:0,z:0,yaw:0,flags:0}]};
 const html=trackWorkspace([track],88,scene);assert.match(html,/value="1" selected>Course model 2/);assert.match(html,/data-preview-add-model="track.big1.course"/);assert.match(html,/data-duplicate-track-object="0"/);assert.match(html,/data-remove-track-object="0"/);assert.match(html,/Add at road point 1/);assert.match(html,/Remove keeps object IDs stable/);
 const empty=trackWorkspace([track],88,{...scene,objects:[]});assert.match(empty,/id="track-add-object"/);assert.ok(!empty.includes('data-remove-track-object'));
});
test('road editor renders local neighbor context, cross-section comparison, route I/O, layers and reshapes route with auto-calibrated geometry',async()=>{
 const {trackWorkspace,deformRoadPoints,shapeRoadSection,diffRoadDraft,build3dTrackView}=await import('../renderer/tracks.mjs');
 const {buildRouteObj,diffRoadAndArcs}=require('../main/track-route.cjs');
 const basePoints=Array.from({length:12},(_,i)=>({x:i*1000,y:i*10,z:0,angle:1024,pitch:0,crossSlope:0,leftWidth:60,rightWidth:60,length:100,arc:0}));
 const deformed=deformRoadPoints(basePoints,basePoints,5,{dx:0,dz:500,dy:40},2);
 assert.equal(deformed[5].z,500);
 assert.equal(deformed[5].y,basePoints[5].y+40);
 assert.ok(deformed[4].z>0&&deformed[4].z<500);
 assert.ok(deformed[6].z>0&&deformed[6].z<500);
 assert.equal(deformed[0].z,0);
 assert.notEqual(deformed[4].angle,basePoints[4].angle);
 assert.ok(deformed[4].length>basePoints[4].length);
 const widened=shapeRoadSection(basePoints,deformed,5,2,'width',20);
 assert.equal(widened[5].leftWidth,80);
 const diff=diffRoadDraft({points:basePoints,arcs:[]},{points:widened,arcs:[]});
 assert.equal(diff.pointCount,6);
 const html=trackWorkspace([{name:'Coast',bank:88,grade:1}],88,{revision:3,road:{points:basePoints,arcs:[]},objects:[]},5,0,{roadRadius:2,roadDraft:{points:widened,arcs:[]}});
 assert.match(html,/class="local-road-map"/);
 assert.match(html,/class="cross-section-svg"/);
 assert.match(html,/Element in Neighbor Context/);
 assert.match(html,/data-export-route="json"/);
 assert.match(html,/data-export-route="obj"/);
 assert.match(html,/data-import-route="88"/);
 assert.match(html,/data-save-road-draft="88"/);
 assert.match(html,/data-layer-preset="road-only"/);
 assert.match(html,/data-layer-preset="isolated"/);
 assert.match(html,/data-layer-preset="full-3d"/);
 assert.match(html,/data-toggle-layer="road"/);
 assert.match(html,/data-toggle-layer="terrain"/);
 assert.match(html,/data-toggle-layer="course"/);
 assert.match(html,/data-road-tool="move"/);
 const rawPreview={
  vertices:[0,0,0,0,1,0,0,0,0, 100,0,0,0,1,0,0,0,0, 0,0,100,0,1,0,0,0,0],
  colors:[255,255,255,255, 255,255,255,255, 255,255,255,255],
  indices:[0,1,2],
  entities:[{bank:'terrain',cell:0,firstIndex:0,indexCount:3},{bank:'course',object:0,firstIndex:0,indexCount:3}]
 };
 const sceneObj={road:{points:basePoints,arcs:[]},objects:[{model:0,x:5000,y:0,z:0}]};
 const roadOnly3d=build3dTrackView(rawPreview,sceneObj,{points:basePoints,arcs:[]},5,2,{road:true,terrain:false,course:false,isolate:false});
 assert.ok(roadOnly3d.data.entities.every(e=>e.bank==='road'));
 assert.equal(roadOnly3d.data.entities.length,12);
 const full3d=build3dTrackView(rawPreview,sceneObj,{points:basePoints,arcs:[]},5,2,{road:true,terrain:true,course:true,isolate:false});
 assert.equal(full3d.data.entities.length,14);
 const obj=buildRouteObj({points:basePoints,arcs:[{x:100,z:200}]},'track.big1',88);
 assert.match(obj,/o road_centerline/);
 assert.match(obj,/o road_ribbon/);
 assert.equal(diffRoadAndArcs({points:basePoints,arcs:[]},widened,[]).length,6);
});
