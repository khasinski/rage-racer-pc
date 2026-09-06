const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {LauncherService}=require('../main/service.cjs');
test('dependencies survive export and prevent activation or edits that break enabled mods',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-dependencies-'));
 try{
  const service=new LauncherService({root:path.join(root,'profile'),bin:path.resolve(__dirname,'../resources/bin'),config:path.resolve(__dirname,'../resources/rage-port.ini')});await service.init();service.state.disc={region:'PAL'};
  async function install(packageId,requires,asset){const folder=path.join(root,packageId);await fs.mkdir(path.join(folder,'raw'),{recursive:true});await fs.writeFile(path.join(folder,'raw',asset),'fixture');await fs.writeFile(path.join(folder,'rage-mod.json'),JSON.stringify({format:1,name:packageId,region:'PAL',version:'1.0',packageId,requires}));return service.importMod(folder);}
  const addon=await install('addon',[{packageId:'core',version:'1.0'}],'asset_012.bin');
  await assert.rejects(service.toggleMod(addon.id,true),/requires core version 1.0/);assert.equal(addon.enabled,false);
  const core=await install('core',[],'asset_010.bin');await service.toggleMod(core.id,true);await service.toggleMod(addon.id,true);
  await assert.rejects(service.toggleMod(core.id,false),/requires core/);assert.equal(core.enabled,true);
  await assert.rejects(service.removeMod(core.id),/requires core/);assert.ok(service.state.mods.includes(core));
  await assert.rejects(service.saveModDetails(core.id,{name:'Core',version:'2.0'}),/requires core/);assert.equal(core.version,'1.0');
  const exported=await service.exportMod(addon.id,root),metadata=JSON.parse(await fs.readFile(path.join(exported,'rage-mod.json')));assert.equal(metadata.packageId,'addon');assert.deepEqual(metadata.requires,[{packageId:'core',version:'1.0'}]);
  // Invalid persisted state must also be rejected at the final launch boundary.
  core.enabled=false;await assert.rejects(service.composeMods(),/requires core/);core.enabled=true;
  await service.toggleMod(addon.id,false);await service.toggleMod(core.id,false);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
