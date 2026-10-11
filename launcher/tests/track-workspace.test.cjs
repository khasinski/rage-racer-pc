const {test}=require('node:test'),assert=require('node:assert/strict');
test('track workspace identifies source, supports point selection and makes read-only limits explicit',async()=>{
  const {trackWorkspace}=await import('../renderer/tracks.mjs');
  const track={name:'Coast <test>',bank:88,grade:1,available:true,blocks:[]};
  const point={x:100,y:-20,z:-100,angle:123,pitch:10,crossSlope:-2,leftWidth:50,rightWidth:80,length:200,arc:0};
  const html=trackWorkspace([track],88,{road:{points:[point,{...point,x:500,z:200}],arcs:[],length:400},blocks:[]},1);
  assert.match(html,/Coast &lt;test&gt;/);
  assert.match(html,/data-track-point="0"/);assert.match(html,/data-track-point="1"/);
  assert.match(html,/Road point 2/);assert.match(html,/not a terrain/);
  assert.match(html,/not connected yet/);assert.ok(!html.includes('NaN'));
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
