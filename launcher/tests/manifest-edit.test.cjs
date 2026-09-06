const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {run,LauncherService}=require('../main/service.cjs');
const bin=path.resolve(__dirname,'../resources/bin');
const tool=path.join(bin,'rage-mod-cli'+(process.platform==='win32'?'.exe':''));
const key='car.0.material.0',properties='lit opaque 0.5 0 1 1 1 1 0 0 0';

test('editing a material refreshes compiled conflicts and survives profile reload',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-edited-claims-'));
 try{
  const options={root:path.join(root,'profile'),bin,config:path.resolve(__dirname,'../resources/rage-port.ini')};
  const service=new LauncherService(options);
  await service.init();service.state.disc={region:'PAL'};
  const mods=[];
  for(const index of [0,1]){
   const source=path.join(root,'source'+index);await fs.mkdir(source);
   await fs.writeFile(path.join(source,'mod.toml'),`[materials]\n"car.${index}.material.0"="${properties}"\n`);
   const mod=await service.importMod(source);await service.toggleMod(mod.id,true);mods.push(mod);
  }
  assert.deepEqual(service.conflicts(),[]);
  const edited='unlit opaque 0.2 0 1 1 1 1 0 0 0';
  await service.saveMaterial(mods[1].id,key,edited);
  assert.ok(mods[1].manifest.resourceClaims.includes('material:'+key));
  assert.deepEqual(service.conflicts().map(c=>c.key),['material:'+key]);
  await assert.rejects(service.composeMods(),/conflict/);
  await service.resolveConflict('material:'+key,mods[1].id);
  const reloaded=new LauncherService(options);await reloaded.init();
  assert.deepEqual(reloaded.conflicts(),[]);
  assert.equal(reloaded.overlaps()[0].winner,mods[1].id);
  const output=await reloaded.composeMods();
  const manifest=JSON.parse(await run(tool,[path.join(output,'mod.toml')]));
  assert.equal(manifest.materials[key],edited);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('compiled material edit preserves the source contract and never overwrites files',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-manifest-edit-'));
 try{
  const source=path.join(root,'source.toml'),output=path.join(root,'edited.toml');
  const text='# keep this comment\r\n[mod]\r\nschema_version=1\r\nid="addon"\r\nrequires=["base"]\r\n[extension]\r\nfuture="keep"\r\n[materials]\r\n"'+key+'"="unlit opaque 0.2 0 1 1 1 1 0 0 0"';
  await fs.writeFile(source,text);
  await run(tool,['--set-material',source,output,key,properties]);
  assert.equal(await fs.readFile(source,'utf8'),text);
  assert.ok((await fs.readFile(output,'utf8')).startsWith(text));
  const manifest=JSON.parse(await run(tool,[output]));
  assert.equal(manifest.id,'addon');assert.equal(manifest.schemaVersion,1);
  assert.deepEqual(manifest.requires,['base']);assert.equal(manifest.materials[key],properties);
  const previous=await fs.readFile(output);
  await assert.rejects(run(tool,['--set-material',source,output,key,properties]));
  assert.deepEqual(await fs.readFile(output),previous);
  await assert.rejects(run(tool,['--set-material',source,source,key,properties]));
  assert.equal(await fs.readFile(source,'utf8'),text);
  for(const [badKey,badProperties] of [['bad_key',properties],[key,'invalid'],['x"\n[mod]\nschema_version=2',properties],[key,'x\n[mod]\nid="oops"']]){
   const rejected=path.join(root,'rejected.toml');
   await assert.rejects(run(tool,['--set-material',source,rejected,badKey,badProperties]));
   await assert.rejects(fs.access(rejected));
  }
  await fs.writeFile(source,'[mod]\nschema_version=2');
  await assert.rejects(run(tool,['--set-material',source,path.join(root,'future.toml'),key,properties]));
  await assert.rejects(fs.access(path.join(root,'future.toml')));
  await assert.rejects(run(tool,['--set-material',path.join(root,'missing'),path.join(root,'missing-output'),key,properties]));
  const fresh=path.join(root,'fresh.toml');
  await run(tool,['--set-material','',fresh,key,properties]);
  assert.equal(JSON.parse(await run(tool,[fresh])).materials[key],properties);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('launcher material editing retains TOML requirements and export identity',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-manifest-service-'));
 try{
  const source=path.join(root,'mod');await fs.mkdir(source);
  const original='[mod]\nid="addon"\nrequires=["base"]\n# extension data must survive\n[future]\nvalue="untouched"\n';
  await fs.writeFile(path.join(source,'mod.toml'),original);
  const service=new LauncherService({root:path.join(root,'profile'),bin,config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();service.state.disc={region:'PAL'};
  const mod=await service.importMod(source);
  await service.saveMaterial(mod.id,key,properties);
  assert.equal(mod.manifest.id,'addon');assert.deepEqual(mod.manifest.requires,['base']);
  const exported=await service.exportMod(mod.id,root);
  const text=await fs.readFile(path.join(exported,'mod.toml'),'utf8');
  assert.ok(text.startsWith(original));
  assert.equal(JSON.parse(await run(tool,[path.join(exported,'mod.toml')])).materials[key],properties);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
