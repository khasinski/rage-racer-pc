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
