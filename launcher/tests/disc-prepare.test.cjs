const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {LauncherService}=require('../main/service.cjs');

// Opt-in owned-data integration: execute the actual game archive reader and C extractor.
for(const [region,variable] of [['PAL','RAGE_LAUNCHER_PAL_CUE'],['NTSC-U','RAGE_LAUNCHER_NTSC_U_CUE'],['NTSC-J','RAGE_LAUNCHER_NTSC_J_CUE']]) {
 test(`launcher prepares and reloads a real ${region} disc`,{skip:!process.env[variable]},async(t)=>{
  const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-disc-prepare-'));
  const source=path.resolve(__dirname,'../..');
  const options={root,bin:process.env.RAGE_LAUNCHER_BUILD_DIR||path.join(source,'build'),config:path.join(source,'rage-port.ini')};
  try {
   const service=new LauncherService(options);await service.init();
   await service.prepare(path.resolve(process.env[variable]));
   assert.equal(service.state.disc.region,region);
   assert.equal(service.busy,null);
   const manifest=await service.assets();
   assert.equal(manifest.entries.length,135);
   assert.equal(new Set(manifest.entries.map(e=>e.index)).size,135);
   for(const entry of manifest.entries) {
    assert.ok(Number.isInteger(entry.index)&&entry.index>=0&&entry.index<135);
    if(entry.present!==false)assert.equal((await fs.stat(path.join(service.state.disc.data,entry.raw))).size,entry.size);
   }
   const restored=new LauncherService(options);await restored.init();
   assert.deepEqual(restored.state.disc,service.state.disc);
   assert.equal((await restored.snapshot()).ready,true);
   const config=await restored.configuration();
   assert.match(config,/renderer\s*=\s*modern/);
   const previous=structuredClone(restored.state);
   const games=await fs.readdir(path.join(root,'games'));
   const invalid=path.join(root,'invalid.bin');
   await fs.writeFile(invalid,Buffer.alloc(2352));
   await assert.rejects(restored.prepare(invalid));
   assert.equal(restored.busy,null);
   assert.deepEqual(restored.state,previous);
   assert.deepEqual(await fs.readdir(path.join(root,'games')),games);
   assert.deepEqual(await restored.assets(),manifest);
   const afterFailure=new LauncherService(options);await afterFailure.init();
   assert.deepEqual(afterFailure.state,previous);
   assert.equal((await afterFailure.snapshot()).ready,true);
   const profile=path.join(root,'launcher.json');
   const saved=await fs.readFile(profile);
   const rename=fs.rename;
   const renameMock=t.mock.method(fs,'rename',async(from,to)=>{
    if(to===profile)throw Object.assign(Error('Injected profile publication failure'),{code:'ENOSPC'});
    return rename(from,to);
   });
   try {
    await assert.rejects(afterFailure.prepare(path.resolve(process.env[variable])),/Injected profile publication failure/);
   } finally {renameMock.mock.restore();}
   assert.deepEqual(afterFailure.state,previous);
   assert.equal(afterFailure.busy,null);
   assert.deepEqual(await fs.readFile(profile),saved);
   assert.deepEqual(await fs.readdir(path.join(root,'games')),games);
   assert.equal((await fs.readdir(root)).some(name=>name.startsWith('launcher.json.')&&name.endsWith('.tmp')),false);
   let canceled=false;
   afterFailure.onChange=snapshot=>{
    if(snapshot.busy==='Preparing the asset library'&&snapshot.canCancel&&!canceled){
     canceled=true;afterFailure.cancel();
    }
   };
   try {
    await assert.rejects(afterFailure.prepare(path.resolve(process.env[variable])),/Operation canceled/);
   } finally {afterFailure.onChange=()=>{};}
   assert.equal(canceled,true);
   assert.equal(afterFailure.busy,null);
   assert.deepEqual(afterFailure.state,previous);
   assert.deepEqual(await fs.readFile(profile),saved);
   assert.deepEqual(await fs.readdir(path.join(root,'games')),games);
   await afterFailure.prepare(path.resolve(process.env[variable]));
   assert.equal(afterFailure.busy,null);
   assert.equal(afterFailure.state.disc.region,region);
   assert.notEqual(afterFailure.state.disc.data,previous.disc.data);
   assert.equal((await afterFailure.snapshot()).ready,true);
  } finally {await fs.rm(root,{recursive:true,force:true});}
 });
}
