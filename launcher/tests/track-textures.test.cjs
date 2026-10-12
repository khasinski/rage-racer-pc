const {test}=require('node:test'),assert=require('node:assert/strict');
const {surfaces}=require('../main/track-meshes.cjs');
test('surface usage follows effective indexed geometry and keeps bank material identity',()=>{
 const vertices=[];for(const word of [3,67,65535,0x12340003])vertices.push(0,0,0,0,1,0,word,0,0);
 const geometry={vertices,indices:[0,1,2,3,3,3,1,1,1],ranges:[[0,3],[3,3],[6,3],[9,0]]};
 assert.deepEqual(surfaces(geometry,0),[{source:3,models:[0,1]},{source:67,models:[0,2]}]);
 assert.deepEqual(surfaces(geometry,1),[{source:3,models:[0,1]}]);assert.throws(()=>surfaces(geometry,3),/non-empty/);assert.throws(()=>surfaces(geometry,9),/non-empty/);
});
test('track texture controls expose thumbnails, explicit scope, reset and identity safely',async()=>{
 const {trackTextureDialog,terrainCellEditor,trackWorkspace}=await import('../renderer/tracks.mjs');
 const html=trackTextureDialog({id:'track.big1.course',part:7,revision:5,surfaces:[{source:67,id:'track.big1.course.material.67',label:'Surface <script>',png:'aGVsbG8=',width:256,height:128,origin:'disc',models:[0,3],overridden:false}]});
 assert.match(html,/Surface &lt;script&gt;/);assert.match(html,/256 × 128 · disc/);assert.match(html,/Used by: Model 1, Model 4/);assert.match(html,/data-import-track-texture="67"/);assert.match(html,/data-export-track-texture="67"/);assert.match(html,/data-reset-track-texture="67" disabled/);assert.match(html,/data-project-material="track.big1.course.material.67"/);assert.match(html,/data-reset-track-material="track.big1.course.material.67" disabled/);assert.match(html,/modern renderer/);assert.match(html,/data-revision="5"/);
 const scene={revision:8,terrain:{models:2,cells:Array(1024).fill(0)}};assert.match(terrainCellEditor(scene,88),/data-track-textures="track.big1.terrain" data-terrain-model-action data-part="0"/);
 assert.match(trackWorkspace([{name:'Coast',bank:88}],88,{revision:5,objects:[{model:7,x:0,y:0,z:0,yaw:0,flags:0}]}),/data-track-textures="track.big1.course" data-part="7"/);
});
