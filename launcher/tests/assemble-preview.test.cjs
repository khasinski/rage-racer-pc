const {test}=require('node:test'),assert=require('node:assert/strict');
const assemble=require('../main/assemble-preview.cjs');
test('assembled instances retain distinct textures, mirrored normals and index offsets',()=>{
 const mesh=texture=>({vertices:[1,2,3,1,0,0,128,0.25,0.75],colors:[64,128,192,255],indices:[0],textures:{0:texture},materials:{0:{id:texture}}});
 const output=assemble({instances:[{part:0,position:[0,0,0],scale:[1,1,1]},{part:2,position:[-10,5,20],scale:[-1,1,-1]},{part:2,position:[10,5,20],scale:[1,1,1]}]},new Map([[0,mesh('paint')],[2,mesh('tire')]]));
 assert.deepEqual(output.indices,[0,1,2]);
 assert.deepEqual(output.colors,[64,128,192,255,64,128,192,255,64,128,192,255]);
 assert.deepEqual(output.vertices.slice(9,18),[-11,7,17,-1,0,-0,129,0.25,0.75]);
 assert.deepEqual(output.textures,{0:'paint',1:'tire'});assert.deepEqual(output.materials,{0:{id:'paint'},1:{id:'tire'}});
 assert.equal(output.vertices[24],129,'the second wheel reuses its own texture source');
});
