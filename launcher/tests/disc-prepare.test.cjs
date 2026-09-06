const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {LauncherService}=require('../main/service.cjs');

// Opt-in owned-data integration: execute the actual game archive reader and C extractor.
for(const [region,variable] of [['PAL','RAGE_LAUNCHER_PAL_CUE'],['NTSC-U','RAGE_LAUNCHER_NTSC_U_CUE'],['NTSC-J','RAGE_LAUNCHER_NTSC_J_CUE']]) {
 test(`launcher prepares and reloads a real ${region} disc`,{skip:!process.env[variable]},async()=>{
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
  } finally {await fs.rm(root,{recursive:true,force:true});}
 });
}
