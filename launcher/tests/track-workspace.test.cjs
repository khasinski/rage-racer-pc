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
