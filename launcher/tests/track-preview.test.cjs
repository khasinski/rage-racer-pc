const {test}=require('node:test'),assert=require('node:assert/strict');
const assemble=require('../main/track-preview.cjs');
test('track preview uses grid placements, GTE scale, rotation and only referenced model ranges',()=>{
 const geometry={ranges:[[0,3],[3,3]],vertices:[4,8,12,1,0,0,1,0,0,8,0,0,1,0,0,1,0,0,0,0,4,1,0,0,1,0,0,999,999,999,0,1,0,2,0,0],colors:Array(16).fill(128),indices:[0,1,2,3,3,3]};
 const out=assemble({terrain:{geometry,instances:[{mesh:0,position:[1024,0,-1024],yaw:0}]},course:{geometry,instances:[{mesh:0,object:7,position:[10,20,30],yaw:1024}]}});
 assert.deepEqual(out.vertices.slice(0,3),[1025,2,-1021]);
 for(const [actual,expected]of out.vertices.slice(27,30).map((n,i)=>[n,[7,22,31][i]]))assert.ok(Math.abs(actual-expected)<1e-8);
 assert.equal(out.vertices.length,54);assert.deepEqual(out.indices,[0,1,2,3,4,5]);
 assert.notEqual(out.vertices[6],out.vertices[33]);assert.equal(out.entities[1].object,7);
 assert.throws(()=>assemble({course:{geometry,instances:[{mesh:2,position:[0,0,0],yaw:0}]}}),/Unknown track preview model/);
});
test('track materials preserve full source IDs and keep bank textures distinct',()=>{
 const make=png=>({ranges:[[0,3]],vertices:[0,0,0,0,1,0,130,0,0,4,0,0,0,1,0,130,1,0,0,0,4,0,1,0,130,0,1],colors:Array(12).fill(128),indices:[0,1,2],textures:{130:{png}},materials:{130:{id:png}}});
 const instances=[{mesh:0,position:[0,0,0],yaw:0}];
 const out=assemble({course:{geometry:make('course'),instances},terrain:{geometry:make('terrain'),instances}});
 assert.deepEqual(out.textures,{0:{png:'course'},1:{png:'terrain'}});
 assert.deepEqual(out.materials,{0:{id:'course'},1:{id:'terrain'}});
 assert.equal(out.vertices[6],0);assert.equal(out.vertices[33],1);
});
