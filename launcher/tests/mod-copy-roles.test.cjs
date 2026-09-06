const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {LauncherService,run}=require('../main/service.cjs');
test('authoritative global conflicts consume compiled file dispositions',()=>{
 const service=Object.create(LauncherService.prototype);
 service.state={resolutions:{}};
 const mods=['first','second'].map(id=>({id,name:id,enabled:true,
  files:['textures/shared.png'],fileDispositions:[2],
  manifest:{textures:{},materials:{},meshes:{}}}));
 // Deliberately disagree with the advisory filename predicate to prove that
 // authoritative role decisions, rather than path prefixes, drive grouping.
 assert.deepEqual(service.overlaps(mods),[]);
 for(const mod of mods)mod.fileDispositions=[1];
 assert.equal(service.overlaps(mods)[0].key,'textures/shared.png');
});
test('runtime composition omits unclaimed meshes and source package identities without losing exports',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-copy-roles-'));
 try{
  const service=new LauncherService({root:path.join(root,'profile'),bin:path.resolve(__dirname,'../resources/bin'),config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();service.state.disc={region:'PAL'};
  for(const id of ['first','second']){
   const source=path.join(root,id);await fs.mkdir(path.join(source,'meshes'),{recursive:true});
   await fs.writeFile(path.join(source,'meshes/unused.rmesh'),id+' work in progress');
   await fs.writeFile(path.join(source,'manifest.json'),JSON.stringify({source:id}));
   await fs.writeFile(path.join(source,'rage-mod.json'),JSON.stringify({format:1,name:id,region:'PAL',packageId:id}));
   await fs.writeFile(path.join(source,'mod.toml'),`[materials]\n"car.${id}"="lit opaque 0.5 0 1 1 1 1 0 0 0"`);
   const mod=await service.importMod(source);await service.toggleMod(mod.id,true);
  }
  assert.equal(service.conflicts().length,0);
  const output=await service.composeMods();
  assert.deepEqual(await fs.readdir(output),['mod.toml']);
  const manifest=JSON.parse(await run(service.tool('rage-mod-cli'),[path.join(output,'mod.toml')]));
  assert.deepEqual(Object.keys(manifest.materials).sort(),['car.first','car.second']);
  assert.deepEqual(manifest.resourceClaims.sort(),['material:car.first','material:car.second']);
  const exported=await service.exportMod(service.state.mods[0].id,root);
  assert.equal(await fs.readFile(path.join(exported,'meshes/unused.rmesh'),'utf8'),'first work in progress');
  assert.equal(JSON.parse(await fs.readFile(path.join(exported,'rage-mod.json'),'utf8')).packageId,'first');
  assert.equal(JSON.parse(await fs.readFile(path.join(exported,'manifest.json'),'utf8')).source,'first');
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
