const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {LauncherService,run}=require('../main/service.cjs');
function pack(){
 const blocks=Array.from({length:11},()=>Buffer.alloc(4));
 blocks[4]=Buffer.alloc(28);blocks[4].writeUInt32LE(1);blocks[4].writeUInt16LE(100,26);
 blocks[5].writeUInt32LE(1);blocks[8]=Buffer.alloc(27);blocks[8].writeUInt32LE(1);blocks[8].writeInt32LE(123,8);blocks[8].fill(0xa7,24);
 const header=Buffer.alloc(44);let offset=44;blocks.forEach((b,i)=>{header.writeUInt32LE(offset,i*4);offset+=b.length;});return Buffer.concat([header,...blocks]);
}
test('track edits compile atomically, preserve unrelated bytes and survive project history',async t=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-track-edit-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));
 const data=path.join(root,'data'),raw=path.join(data,'raw');await fs.mkdir(raw,{recursive:true});
 const original=pack(),source=path.join(raw,'asset_088.bin');await fs.writeFile(source,original);
 const archive=path.join(root,'RAGE.BIN'),config=path.join(root,'default.ini');await fs.writeFile(archive,'track source');await fs.writeFile(config,'[video]\nrenderer=modern\n');
 const s=new LauncherService({root:path.join(root,'editor'),bin:path.resolve(__dirname,'../resources/bin'),config,modsRoot:path.join(root,'mods')});await s.init();s.requireReady=()=>{};s.state.disc={archive,region:'PAL',path:archive,data};
 const output=path.join(root,'invalid.bin');
 await assert.rejects(run(s.tool('rage-mod-cli'),['--edit-track-stdin',source,output],{input:Buffer.from(JSON.stringify([[0,'x',99],[0,'flags',16]]))}),/Invalid track object edit/);
 await assert.rejects(fs.stat(output),{code:'ENOENT'});assert.deepEqual(await fs.readFile(source),original);
 await assert.rejects(run(s.tool('rage-mod-cli'),['--edit-track-stdin',source,output],{input:Buffer.from('[[0,"x",18446744073709551615]]')}),/Invalid track object edit/);
 await assert.rejects(fs.stat(output),{code:'ENOENT'});
 let p=await s.project();
 await assert.rejects(s.saveTrackObject(88,0,{model:9},p.revision),/Invalid track object edit/);
 assert.equal((await s.project()).revision,p.revision);
 p=await s.saveTrackObject(88,0,{x:-200,yaw:1024},p.revision);
 await assert.rejects(s.saveTrackObject(88,0,{x:0},p.revision-1),/Project changed/);
 p=await s.projectHistory('undo',p.revision);assert.equal(p.changes.length,0);p=await s.projectHistory('redo',p.revision);
 const preview=await s.trackScene(88);assert.equal(preview.objects[0].x,-200);assert.equal(preview.revision,p.revision);
 p=await s.applyProject(p.revision);const pointer=await fs.readFile(path.join(s.modsRoot,'active.json'),'utf8'),identity=JSON.parse(pointer);
 const applied=path.join(s.modsRoot,'builds',identity.build,'raw/asset_088.bin');
 const bytes=await fs.readFile(applied),offset=original.readUInt32LE(32);
 assert.deepEqual(bytes.subarray(0,offset),original.subarray(0,offset));
 assert.deepEqual(bytes.subarray(offset+12),original.subarray(offset+12));
 const scene=JSON.parse(await run(s.tool('rage-mod-cli'),['--track-scene',applied]));
 assert.equal(scene.objects[0].x,-200);assert.equal(scene.objects[0].yaw,1024);
 assert.deepEqual(await fs.readFile(source),original);
 p=await s.saveRoadPoint(88,0,{left_width:250,y:-42},p.revision);
 await assert.rejects(s.saveRoadPoint(88,0,{length:0},p.revision),/Invalid track object edit/);
 assert.equal((await s.project()).revision,p.revision);
 assert.equal((await s.trackScene(88)).road.points[0].leftWidth,250);
 p=await s.projectHistory('undo',p.revision);assert.equal((await s.trackScene(88)).road.points[0].leftWidth,0);
 p=await s.projectHistory('redo',p.revision);
 p=await s.applyProject(p.revision);
 const roadPointer=await fs.readFile(path.join(s.modsRoot,'active.json'),'utf8');
 const roadBuild=path.join(s.modsRoot,'builds',JSON.parse(roadPointer).build,'raw/asset_088.bin');
 const roadScene=JSON.parse(await run(s.tool('rage-mod-cli'),['--track-scene',roadBuild]));
 assert.equal(roadScene.road.points[0].leftWidth,250);assert.equal(roadScene.road.points[0].y,-42);
 assert.deepEqual(await fs.readFile(source),original);
 p=await s.editProject([{kind:'track',id:'track.big1.object.0',field:'model',value:9}],p.revision);
 await assert.rejects(s.applyProject(p.revision),/Invalid track object edit/);
 assert.equal(await fs.readFile(path.join(s.modsRoot,'active.json'),'utf8'),roadPointer);
});
test('road edits preserve all other blocks and reject invalid routes before creating output',async t=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-road-edit-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));
 const source=path.join(root,'source.bin'),output=path.join(root,'road.bin'),original=pack();await fs.writeFile(source,original);
 const tool=path.resolve(__dirname,'../resources/bin/rage-mod-cli');
 const grouped=require('../main/track-edits.cjs').group([{kind:'track',id:'track.big1.road.0',field:'left_width',value:250}]);
 assert.deepEqual(grouped.get(88),[[0,'road.leftWidth',250]]);
 await run(tool,['--edit-track-stdin',source,output],{input:Buffer.from(JSON.stringify([[0,'road.leftWidth',250],[0,'road.y',-42]]))});
 const bytes=await fs.readFile(output),start=original.readUInt32LE(16),end=original.readUInt32LE(20);
 assert.deepEqual(bytes.subarray(0,start),original.subarray(0,start));assert.deepEqual(bytes.subarray(end),original.subarray(end));
 const scene=JSON.parse(await run(tool,['--track-scene',output]));assert.equal(scene.road.points[0].leftWidth,250);assert.equal(scene.road.points[0].y,-42);
 await fs.rm(output);
 for(const edit of [[0,'road.length',0],[0,'road.arc',17],[0,'road.y',32768],[1,'road.x',10]]){
  await assert.rejects(run(tool,['--edit-track-stdin',source,output],{input:Buffer.from(JSON.stringify([[0,'road.leftWidth',123],edit]))}),/Invalid track object edit/);
  await assert.rejects(fs.stat(output),{code:'ENOENT'});
 }
 assert.deepEqual(await fs.readFile(source),original);
});
test('curve center edits preserve reserved bytes and reject out-of-table indices',async t=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-arc-edit-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));
 const original=pack(),start=original.readUInt32LE(16),end=original.readUInt32LE(20),extra=Buffer.alloc(12);extra.writeInt32LE(123,8);
 const bytes=Buffer.concat([original.subarray(0,end),extra,original.subarray(end)]);for(let i=5;i<11;i++)bytes.writeUInt32LE(original.readUInt32LE(i*4)+12,i*4);
 bytes.writeUInt16LE(1,start+24);
 const source=path.join(root,'source.bin'),out=path.join(root,'out.bin');await fs.writeFile(source,bytes);
 const tool=path.resolve(__dirname,'../resources/bin/rage-mod-cli');
 await run(tool,['--edit-track-stdin',source,out],{input:Buffer.from('[[0,"arc.x",-999],[0,"arc.z",456]]')});
 const result=await fs.readFile(out);assert.deepEqual(result.subarray(0,end),bytes.subarray(0,end));assert.deepEqual(result.subarray(end+8),bytes.subarray(end+8));
 const scene=JSON.parse(await run(tool,['--track-scene',out]));assert.deepEqual(scene.road.arcs[0],{x:-999,z:456,reserved:123});
 await fs.rm(out);await assert.rejects(run(tool,['--edit-track-stdin',source,out],{input:Buffer.from('[[0,"arc.x",8],[1,"arc.x",10]]')}),/Invalid track object edit/);await assert.rejects(fs.stat(out),{code:'ENOENT'});
 assert.deepEqual(require('../main/track-edits.cjs').group([{kind:'track',id:'track.big1.arc.0',field:'x',value:99}]).get(88),[[0,'arc.x',99]]);
 const data=path.join(root,'data');await fs.mkdir(path.join(data,'raw'),{recursive:true});await fs.writeFile(path.join(data,'raw/asset_088.bin'),bytes);
 const archive=path.join(root,'RAGE.BIN'),config=path.join(root,'default.ini');await fs.writeFile(archive,'arc source');await fs.writeFile(config,'[video]\nrenderer=modern\n');
 const service=new LauncherService({root:path.join(root,'editor'),bin:path.resolve(__dirname,'../resources/bin'),config,modsRoot:path.join(root,'mods')});await service.init();service.requireReady=()=>{};service.state.disc={archive,region:'PAL',path:archive,data};
 let project=await service.project();project=await service.saveRoadArc(88,0,{x:-555},project.revision);
 assert.equal((await service.trackScene(88)).road.arcs[0].x,-555);
 await assert.rejects(service.saveRoadArc(88,1,{z:0},project.revision),/Invalid track object edit/);
 assert.equal((await service.project()).revision,project.revision);
 project=await service.projectHistory('undo',project.revision);assert.equal((await service.trackScene(88)).road.arcs[0].x,0);
 project=await service.projectHistory('redo',project.revision);assert.equal((await service.trackScene(88)).road.arcs[0].x,-555);

});
test('terrain cell model edits preserve region bits and every other byte',async t=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-cell-edit-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));
 const original=pack(),start=original.readUInt32LE(28),end=original.readUInt32LE(32),terrain=Buffer.alloc(6160);
 terrain.writeUInt16LE(0xfc00);terrain.writeUInt32LE(1,6144);terrain.writeUInt32LE(12,6148);terrain.writeUInt32LE(12,6152);
 const bytes=Buffer.concat([original.subarray(0,start),terrain,original.subarray(end)]);for(let i=8;i<11;i++)bytes.writeUInt32LE(original.readUInt32LE(i*4)+terrain.length-(end-start),i*4);
 const source=path.join(root,'source.bin'),out=path.join(root,'out.bin'),tool=path.resolve(__dirname,'../resources/bin/rage-mod-cli');await fs.writeFile(source,bytes);
 await run(tool,['--edit-track-stdin',source,out],{input:Buffer.from('[[0,"cell.model",1023]]')});const result=await fs.readFile(out);
 const scene=JSON.parse(await run(tool,['--track-scene',out]));assert.equal(scene.terrain.models,1);assert.equal(scene.terrain.cells.length,1024);assert.equal(scene.terrain.cells[0],65535);assert.equal(result.readUInt16LE(start),0xffff);assert.deepEqual(result.subarray(0,start),bytes.subarray(0,start));assert.deepEqual(result.subarray(start+2),bytes.subarray(start+2));
 await fs.rm(out);for(const edit of [[0,'cell.model',1],[1024,'cell.model',0]]){await assert.rejects(run(tool,['--edit-track-stdin',source,out],{input:Buffer.from(JSON.stringify([edit]))}),/Invalid track object edit/);await assert.rejects(fs.stat(out),{code:'ENOENT'});}
 assert.deepEqual(require('../main/track-edits.cjs').group([{kind:'track',id:'track.big1.cell.0',field:'model',value:1023}]).get(88),[[0,'cell.model',1023]]);
});
