const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {run}=require('../main/service.cjs');
test('headless track export rejects malformed source without modifying existing outputs',async t=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-track-export-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));
 const source=path.join(root,'bad.bin'),config=path.join(root,'config.ini');await fs.writeFile(source,Buffer.alloc(44));await fs.writeFile(config,'');
 for(const name of ['course.rmesh','terrain.rmesh'])await fs.writeFile(path.join(root,name),'existing');
 const exe=path.resolve(__dirname,'../resources/bin/rage-racer'+(process.platform==='win32'?'.exe':''));
 await assert.rejects(run(exe,['--config',config,'--set','tools.track_export='+source,'--set','tools.track_bank=88','--set','tools.track_output='+root],{cwd:root,env:{...process.env,PSYZ_HEADLESS:'1'}}));
 for(const name of ['course.rmesh','terrain.rmesh'])assert.equal(await fs.readFile(path.join(root,name),'utf8'),'existing');
 assert.deepEqual((await fs.readdir(root)).sort(),['bad.bin','config.ini','course.rmesh','terrain.rmesh']);
});
test('bank preview preserves empty submeshes and keeps single-part validation strict',async t=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-bank-preview-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));
 const tool=name=>path.resolve(__dirname,'../resources/bin',name+(process.platform==='win32'?'.exe':''));
 const obj=path.join(root,'triangle.obj'),single=path.join(root,'single.rmesh'),bank=path.join(root,'bank.rmesh');
 await fs.writeFile(obj,'v 0 0 0\nv 1 0 0\nv 0 1 0\nvt 0 0\nvn 0 0 1\nusemtl rage_65535\nf 1/1/1 2/1/1 3/1/1\n');
 await run(tool('rage-mesh-obj'),['import',obj,single]);
 const bytes=await fs.readFile(single),header=Buffer.from(bytes.subarray(0,24)),offsets=Buffer.alloc(12);
 header.writeUInt32LE(2,12);offsets.writeUInt32LE(bytes.readUInt32LE(28),8);
 await fs.writeFile(bank,Buffer.concat([header,offsets,bytes.subarray(32)]));
 const preview=JSON.parse(await run(tool('rage-mod-cli'),['--preview-bank',bank]));
 assert.deepEqual(preview.ranges,[[0,0],[0,3]]);assert.equal(preview.vertices.length,27);assert.equal(preview.indices.length,3);
 await assert.rejects(run(tool('rage-mod-cli'),['--mesh',bank]),/single-part/);
 await assert.rejects(run(tool('rage-mod-cli'),['--preview',bank]),/single-part/);
 assert.equal(JSON.parse(await run(tool('rage-mod-cli'),['--mesh-bank',bank])).vertices,3);
});
