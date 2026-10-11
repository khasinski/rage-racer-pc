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
 let p=await s.project();p=await s.editProject([{kind:'track',id:'track.big1.object.0',field:'x',value:-200},{kind:'track',id:'track.big1.object.0',field:'yaw',value:1024}],p.revision);
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
 p=await s.editProject([{kind:'track',id:'track.big1.object.0',field:'model',value:9}],p.revision);
 await assert.rejects(s.applyProject(p.revision),/Invalid track object edit/);
 assert.equal(await fs.readFile(path.join(s.modsRoot,'active.json'),'utf8'),pointer);
});
