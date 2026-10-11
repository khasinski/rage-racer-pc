const {test}=require('node:test'),assert=require('node:assert/strict');
test('scene picking selects nearest object and respects terrain occlusion and view transforms',async()=>{
 const {pickScene}=await import('../renderer/scene-pick.mjs');
 const frame={center:[0,0,0],scale:1},view={yaw:0,pitch:0,zoom:1,aspect:1};
 const vertices=[],indices=[],entities=[];
 const add=(z,bank,object)=>{const firstIndex=indices.length;for(const [x,y]of [[-1,-1],[1,-1],[0,1]]){indices.push(vertices.length/9);vertices.push(x,y,z,0,0,1,0,0,0);}entities.push({bank,object,firstIndex,indexCount:3});};
 add(.5,'course',3);add(-.5,'course',8);
 const data={vertices,indices,entities};
 assert.equal(pickScene(data,frame,view,0,0).object,8);assert.equal(pickScene(data,frame,view,2,0),null);
 add(-.8,'terrain');assert.equal(pickScene(data,frame,view,0,0).bank,'terrain');
 const rotated={vertices:[1,-1,-1,0,0,1,0,0,0,1,-1,1,0,0,1,0,0,0,1,1,0,0,0,1,0,0,0],indices:[0,1,2],entities:[{bank:'course',object:4,firstIndex:0,indexCount:3}]};
 assert.equal(pickScene(rotated,frame,{...view,yaw:Math.PI/2},0,0).object,4);
 assert.equal(pickScene(rotated,frame,view,0,0),null);
});
