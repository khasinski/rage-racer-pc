const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {LauncherService}=require('../main/service.cjs');
test('editing mod identity persists, exports and rolls back without changing compatibility or assets',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-mod-details-'));
 try{
  const source=path.join(root,'source');await fs.mkdir(path.join(source,'raw'),{recursive:true});await fs.writeFile(path.join(source,'raw/asset_010.bin'),'fixture');
  const service=new LauncherService({root:path.join(root,'profile'),bin:path.resolve(__dirname,'../resources/bin'),config:path.resolve(__dirname,'../resources/rage-port.ini')});await service.init();service.state.disc={region:'PAL'};
  const mod=await service.importMod(source),files=[...mod.files];
  await service.saveModDetails(mod.id,{name:'My car',author:'Author',version:'1.0',description:'Line one\nLine two'});
  assert.equal(JSON.parse(await fs.readFile(path.join(service.root,'launcher.json'))).mods[0].name,'My car');
  assert.equal(mod.region,'PAL');assert.deepEqual(mod.files,files);assert.equal(mod.enabled,false);
  const exported=await service.exportMod(mod.id,root);assert.equal(JSON.parse(await fs.readFile(path.join(exported,'rage-mod.json'))).author,'Author');
  await assert.rejects(service.saveModDetails(mod.id,{name:'',author:'a'}),/name/);
  await assert.rejects(service.saveModDetails(mod.id,{name:'Valid',region:'NTSC-U'}),/Unknown/);
  const persisted=await fs.readFile(path.join(service.root,'launcher.json'),'utf8');
  await assert.rejects(service.saveModDetails(mod.id,{name:'\ud800'}),/Invalid mod metadata/);
  assert.equal(mod.name,'My car');
  assert.equal(await fs.readFile(path.join(service.root,'launcher.json'),'utf8'),persisted);
  service.persist=async()=>{throw Error('disk full');};await assert.rejects(service.saveModDetails(mod.id,{name:'Lost edit'}),/disk full/);assert.equal(mod.name,'My car');assert.equal(service.busy,null);
  service.game={};await assert.rejects(service.saveModDetails(mod.id,{name:'While playing'}),/running/);assert.equal(mod.name,'My car');
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
