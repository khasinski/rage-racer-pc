const {test}=require('node:test');const assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {LauncherService}=require('../main/service.cjs');
test('mesh mods validate, preserve material edits, resolve conflicts and export',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-mesh-mod-'));
 const {run}=require('../main/service.cjs');
 try{
  const bin=path.resolve(__dirname,'../resources/bin');
  const service=new LauncherService({root:path.join(root,'profile'),bin,config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();service.state.disc={region:'NTSC-U'};
  const key='car.player.10.part.0';
  for(const [name,x] of [['first',1],['second',2]]){
   const source=path.join(root,name);await fs.mkdir(path.join(source,'meshes'),{recursive:true});
   const obj=path.join(root,name+'.obj');
   await fs.writeFile(obj,`v 0 0 0 1 0 0\nv ${x} 0 0\nv 0 1 0\nvt 0 0\nvn 0 0 1\nusemtl rage_0\nf 1/1/1 2/1/1 3/1/1\n`);
   await run(service.tool('rage-mesh-obj'),['import',obj,path.join(source,'meshes/body.rmesh')]);
   await fs.writeFile(path.join(source,'mod.toml'),`[meshes]\n"${key}" = "meshes/body.rmesh"\n`);
   await service.importMod(source);await service.toggleMod(service.state.mods.at(-1).id,true);
  }
  const [first,second]=service.state.mods;
  await service.saveMaterial(first.id,'car.0.material.0','lit opaque 0.5 0 1 1 1 1 0 0 0');
  assert.equal(first.manifest.meshes[key],'meshes/body.rmesh');
  assert.deepEqual(service.conflicts().map(c=>c.key),['mesh:'+key]);
  await assert.rejects(service.composeMods(),/conflict/);
  await service.resolveConflict('mesh:'+key,second.id);
  const output=await service.composeMods();
  const manifest=JSON.parse(await run(service.tool('rage-mod-cli'),[path.join(output,'mod.toml')]));
  assert.deepEqual(await fs.readFile(path.join(output,manifest.meshes[key])),await fs.readFile(path.join(root,'second/meshes/body.rmesh')));
  service.carTool=async()=>JSON.stringify({materials:[]});
  service.state.disc.data=root;await fs.mkdir(path.join(root,'textures'));
  const preview=await service.previewCar(key,second.id);
  assert.equal(preview.vertices.length,27);
  assert.deepEqual(preview.colors.slice(0,4),[255,0,0,255],'native preview preserves authored vertex color');
  assert.deepEqual(preview.indices,[0,1,2]);
  assert.equal(preview.vertices[9],2,'preview reads the selected mod geometry');
  await fs.mkdir(path.join(root,'raw'));
  const modelHeader=Buffer.alloc(128);modelHeader.writeUInt32LE(40,32);modelHeader.writeUInt32LE(80,36);
  await fs.writeFile(path.join(root,'raw/asset_010.bin'),modelHeader);
  service.carTool=async args=>{
   const output=args.find(a=>a.startsWith('tools.car_output=')).slice('tools.car_output='.length);
   await fs.copyFile(path.join(root,'first/meshes/body.rmesh'),output);
   return JSON.stringify({materials:[]});
  };
  const assembled=await service.previewWholeCar(10,0,false,second.id);
  assert.equal(assembled.vertices[9],2,'body replacement is used in the assembled preview');
  assert.equal(assembled.vertices[27+9],1,'unreplaced rear-wheel geometry comes from the base');
  assert.equal(assembled.indices.length,12,'body, rear pair and both front wheels are retained');
  assert.equal(assembled.modName,second.name);
  await assert.rejects(service.previewWholeCar(10,0,false,'missing'),/Unknown mod/);
  await assert.rejects(service.previewCar(key,'missing'),/Unknown mod model/);
  const exported=await service.exportMod(first.id,root);
  assert.deepEqual(await fs.readFile(path.join(exported,'meshes/body.rmesh')),await fs.readFile(path.join(root,'first/meshes/body.rmesh')));
  await fs.writeFile(path.join(root,'first/meshes/body.rmesh'),'broken');
  await assert.rejects(service.importMod(path.join(root,'first')),/RRMESH/);
  assert.equal(service.state.mods.length,2);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
test('real material mods import, edit, compose and conflict without touching the source',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-service-test-'));
 try{
  const source=path.join(root,'source');await fs.mkdir(source);
  const original='[mod]\nid = "test"\n[materials]\n"car.0.material.0" = "lit opaque 0.35 0 1 1 1 1 0 0 0"\n';
  await fs.writeFile(path.join(source,'mod.toml'),original);
  const service=new LauncherService({root:path.join(root,'profile'),bin:path.resolve(__dirname,'../resources/bin'),config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();service.state.disc={region:'NTSC-U'};
  await service.importMod(source);const id=service.state.mods[0].id;
  assert.equal(service.state.mods[0].enabled,false);
  await service.saveMaterial(id,'car.0.material.0','lit opaque 0.8 0 1 1 1 1 0 0 0');
  await assert.rejects(service.saveMaterial(id,'car.0.material.0','lit opaque 9 0 1 1 1 1 0 0 0'));
  await service.toggleMod(id,true);const output=await service.composeMods();
  assert.match(await fs.readFile(path.join(output,'mod.toml'),'utf8'),/opaque 0.8/);
  assert.equal(await fs.readFile(path.join(source,'mod.toml'),'utf8'),original);
  await service.importMod(source);await service.toggleMod(service.state.mods[1].id,true);
  assert.equal(service.conflicts().length,1);await assert.rejects(service.composeMods(),/conflict/);
  await service.resolveConflict('material:car.0.material.0',id);
  assert.equal(service.conflicts().length,0);
  const resolved=await service.composeMods();
  assert.match(await fs.readFile(path.join(resolved,'mod.toml'),'utf8'),/opaque 0.8/);
  await service.importMod(source);await service.toggleMod(service.state.mods[2].id,true);
  assert.equal(service.conflicts().length,1,'a new provider requires a new decision');
  await assert.rejects(service.resolveConflict('material:car.0.material.0','not-a-mod'));
  await service.toggleMod(service.state.mods[2].id,false);
  assert.equal(service.conflicts().length,0,'the original provider set retains its decision');
  await service.toggleMod(service.state.mods[1].id,false);assert.equal(service.conflicts().length,0);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('semantic texture providers keep their own pixels even with identical filenames',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-texture-priority-'));
 try{
  const service=new LauncherService({root:path.join(root,'profile'),bin:path.resolve(__dirname,'../resources/bin'),config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();service.state.disc={region:'NTSC-U'};
  for(const [name,key]of [['first','car.0.material.0'],['second','car.1.material.0']]){
   const dir=path.join(root,name);await fs.mkdir(path.join(dir,'textures'),{recursive:true});
   await fs.writeFile(path.join(dir,'textures','body.png'),name);
   await fs.writeFile(path.join(dir,'mod.toml'),`[mod]\nid = "${name}"\n[textures]\n"${key}" = "textures/body.png"\n`);
   await service.importMod(dir);await service.toggleMod(service.state.mods.at(-1).id,true);
  }
  assert.equal(service.conflicts().length,0,'different semantic IDs do not conflict over source filenames');
  const composed=await service.composeMods();
  const {run}=require('../main/service.cjs');
  const manifest=JSON.parse(await run(service.tool('rage-mod-cli'),[path.join(composed,'mod.toml')]));
  assert.equal(await fs.readFile(path.join(composed,manifest.textures['car.0.material.0']),'utf8'),'first');
  assert.equal(await fs.readFile(path.join(composed,manifest.textures['car.1.material.0']),'utf8'),'second');
  const loaded=new LauncherService({root:service.root,bin:service.bin,config:service.baseConfig});
  await loaded.init();assert.equal(loaded.state.mods.length,2);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('legacy texture priorities preserve PNG/JSON pairs and merge unrelated assets',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-legacy-mods-'));
 try{
  const service=new LauncherService({root:path.join(root,'profile'),bin:path.resolve(__dirname,'../resources/bin'),config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();const data=path.join(root,'data');await fs.mkdir(path.join(data,'raw'),{recursive:true});await fs.writeFile(path.join(data,'raw/asset_000.bin'),'recognition marker');service.state.disc={region:'NTSC-U',data};
  for(const [name,asset,stem]of [['first',10,'body'],['other',12,'body'],['replacement',10,'renamed']]){
   const source=path.join(root,name);await fs.mkdir(path.join(source,'textures'),{recursive:true});
   await fs.writeFile(path.join(source,'textures/index.txt'),`${asset} ${stem}.json\n`);
   await fs.writeFile(path.join(source,`textures/${stem}.json`),JSON.stringify({provider:name}));
   await fs.writeFile(path.join(source,`textures/${stem}.png`),name);
   // The same PNG can also serve a semantic texture without following legacy priorities.
   await fs.writeFile(path.join(source,'mod.toml'),`[textures]\n"car.${asset}.material.${name==='replacement'?1:0}" = "textures/${stem}.png"\n`);
   await service.importMod(source);await service.toggleMod(service.state.mods.at(-1).id,true);
  }
  const [first,other,replacement]=service.state.mods;
  assert.deepEqual(service.conflicts().map(c=>c.key),['legacy-textures:asset-10']);
  await assert.rejects(service.composeMods(),/conflict/);
  await service.resolveConflict('legacy-textures:asset-10',replacement.id);
  const target=await service.composeMods();
  assert.equal(await fs.readFile(path.join(target,'raw/asset_000.bin'),'utf8'),'recognition marker');
  const lines=(await fs.readFile(path.join(target,'textures/index.txt'),'utf8')).trim().split('\n');
  assert.equal(lines.length,2);
  for(const line of lines){
   const [asset,stem]=line.split(' '),expected=asset==='10'?'replacement':'other';
   const metadata=JSON.parse(await fs.readFile(path.join(target,'textures',stem)));
   assert.equal(metadata.provider,expected);
   assert.equal(await fs.readFile(path.join(target,'textures',stem.replace(/\.json$/,'.png')),'utf8'),expected);
  }
  const {run}=require('../main/service.cjs');
  const manifest=JSON.parse(await run(service.tool('rage-mod-cli'),[path.join(target,'mod.toml')]));
  assert.equal(await fs.readFile(path.join(target,manifest.textures['car.10.material.0']),'utf8'),'first');
  // Upgrade older profiles by deriving bundle metadata from the installed index.
  for(const mod of service.state.mods)delete mod.legacyTextures;await service.persist();
  const loaded=new LauncherService({root:service.root,bin:service.bin,config:service.baseConfig});await loaded.init();
  assert.equal(loaded.overlaps()[0].winner,replacement.id);
  assert.equal(loaded.state.mods.find(m=>m.id===other.id).legacyTextures[0].asset,12);
  const bad=path.join(root,'bad');await fs.mkdir(path.join(bad,'textures'),{recursive:true});
  for(const index of ['10 missing.json\n','135 body.json\n','10 ../body.json\n']){
   await fs.writeFile(path.join(bad,'textures/index.txt'),index);
   await assert.rejects(service.importMod(bad),/legacy texture/);
  }
  assert.equal(service.state.mods.length,3);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('failed profile writes preserve mod enablement, membership and files',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-mod-persist-'));
 try{
  const source=path.join(root,'source');await fs.mkdir(source);
  await fs.writeFile(path.join(source,'mod.toml'),'[materials]\n"car.0.material.0" = "lit opaque 0.3 0 1 1 1 1 0 0 0"\n');
  const service=new LauncherService({root:path.join(root,'profile'),bin:path.resolve(__dirname,'../resources/bin'),config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();service.state.disc={region:'NTSC-U'};await service.importMod(source);
  const mod=service.state.mods[0],profile=await fs.readFile(path.join(service.root,'launcher.json'));
  const persist=service.persist;service.persist=async()=>{throw Object.assign(Error('Disk full'),{code:'ENOSPC'});};
  await assert.rejects(service.toggleMod(mod.id,true),/Disk full/);assert.equal(mod.enabled,false);
  await assert.rejects(service.removeMod(mod.id),/Disk full/);assert.equal(service.state.mods[0],mod);
  assert.deepEqual(await fs.readFile(path.join(service.root,'launcher.json')),profile);
  assert.equal((await fs.stat(path.join(service.root,'mods',mod.id,'mod.toml'))).isFile(),true);
  assert.equal(service.busy,null);
  service.persist=persist;await service.toggleMod(mod.id,true);assert.equal(mod.enabled,true);
  await service.removeMod(mod.id);assert.equal(service.state.mods.length,0);
  await assert.rejects(fs.access(path.join(service.root,'mods',mod.id)));
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('settings retain the last saved values when persistence fails',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-settings-persist-'));
 try{
  const service=new LauncherService({root,bin:path.resolve(__dirname,'../resources/bin'),config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();service.state.disc={region:'NTSC-U'};
  await service.saveSettings({'video.internal_scale':'2'});
  const before=await fs.readFile(path.join(root,'launcher.json')),persist=service.persist;
  service.persist=async()=>{throw Object.assign(Error('Disk full'),{code:'ENOSPC'});};
  await assert.rejects(service.saveSettings({'video.internal_scale':'4'}),/Disk full/);
  assert.equal((await service.snapshot()).settings['video.internal_scale'],'2');
  assert.deepEqual(await fs.readFile(path.join(root,'launcher.json')),before);
  service.persist=persist;await service.saveSettings({'video.internal_scale':'4'});
  const reloaded=new LauncherService({root,bin:service.bin,config:service.baseConfig});await reloaded.init();
  assert.equal(reloaded.state.settings['video.internal_scale'],'4');
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('failed material persistence restores the original manifest or removes a newly created one',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-material-rollback-'));
 try{
  for(const hasManifest of [true,false]){
   const folder=path.join(root,String(hasManifest)),source=path.join(folder,'source');await fs.mkdir(path.join(source,'raw'),{recursive:true});await fs.writeFile(path.join(source,'raw/asset_010.bin'),'asset');
   const original='[materials]\n"car.0.material.0" = "lit opaque 0.2 0 1 1 1 1 0 0 0"\n';
   if(hasManifest)await fs.writeFile(path.join(source,'mod.toml'),original);
   const service=new LauncherService({root:path.join(folder,'profile'),bin:path.resolve(__dirname,'../resources/bin'),config:path.resolve(__dirname,'../resources/rage-port.ini')});await service.init();service.state.disc={region:'NTSC-U'};await service.importMod(source);
   const mod=service.state.mods[0],before=JSON.stringify(mod),profile=await fs.readFile(path.join(service.root,'launcher.json'));
   service.persist=async()=>{throw Error('Disk full');};
   await assert.rejects(service.saveMaterial(mod.id,'car.0.material.0','lit opaque 0.8 0 1 1 1 1 0 0 0'),/Disk full/);
   assert.equal(JSON.stringify(mod),before);assert.deepEqual(await fs.readFile(path.join(service.root,'launcher.json')),profile);
   const manifest=path.join(service.root,'mods',mod.id,'mod.toml');
   if(hasManifest)assert.equal(await fs.readFile(manifest,'utf8'),original);else await assert.rejects(fs.access(manifest));
   assert.equal((await fs.readdir(service.root)).some(n=>n.startsWith('material-')),false);
  }
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('settings and priority saves hold the profile lock until persistence finishes',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-profile-lock-'));
 try{
  const service=new LauncherService({root,bin:path.resolve(__dirname,'../resources/bin'),config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();service.state.disc={region:'NTSC-U'};
  service.state.mods=['a','b'].map(id=>({id,name:id,enabled:true,files:[],manifest:{materials:{'car.0.material.0':'lit opaque 0.3 0 1 1 1 1 0 0 0'},textures:{},meshes:{}}}));
  const key=service.overlaps()[0].key,persist=service.persist;
  for(const save of [()=>service.saveSettings({'video.internal_scale':'3'}),()=>service.resolveConflict(key,'a')]){
   let release,entered;const gate=new Promise(resolve=>release=resolve),started=new Promise(resolve=>entered=resolve);
   service.persist=async()=>{entered();await gate;await persist.call(service);};
   const pending=save();await started;
   try{
    assert.equal(service.busy.cancellable,false);
    await assert.rejects(service.saveSettings({'video.internal_scale':'4'}),/current operation/);
    await assert.rejects(service.resolveConflict(key,'b'),/current operation/);
    await assert.rejects(service.toggleMod('a',false),/current operation/);
    await assert.rejects(service.launch(),/current operation/);
   }finally{release();await pending;}
   assert.equal(service.busy,null);
  }
  const profile=JSON.parse(await fs.readFile(path.join(root,'launcher.json'),'utf8'));
  assert.equal(profile.settings['video.internal_scale'],'3');assert.equal(profile.resolutions[key].winner,'a');
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('exported mods preserve identity, description and disc compatibility when reimported',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-mod-metadata-'));
 try{
  const source=path.join(root,'Readable mod name');await fs.mkdir(path.join(source,'raw'),{recursive:true});await fs.writeFile(path.join(source,'raw/asset_010.bin'),'replacement');
  const details={author:'Mod author',version:'1.2 beta',description:'Body and wheels.\nIncludes <custom> glass.'};
  await fs.writeFile(path.join(source,'rage-mod.json'),JSON.stringify({format:1,name:'Readable mod name',region:'NTSC-U',...details}));
  const service=new LauncherService({root:path.join(root,'profile'),bin:path.resolve(__dirname,'../resources/bin'),config:path.resolve(__dirname,'../resources/rage-port.ini')});await service.init();service.state.disc={region:'NTSC-U'};
  const original=await service.importMod(source),exported=await service.exportMod(original.id,root);
  service.state.disc={region:'PAL'};const imported=await service.importMod(exported);
  assert.equal(imported.name,'Readable mod name');assert.equal(imported.region,'NTSC-U');assert.equal(imported.enabled,false);
  for(const [field,value] of Object.entries(details))assert.equal(imported[field],value);
  await assert.rejects(service.toggleMod(imported.id,true),/different disc region/);
  const before=service.state.mods.length;
  for(const invalid of [{author:12},{version:'bad\nversion'},{description:'x'.repeat(1201)}]){
   await fs.writeFile(path.join(exported,'rage-mod.json'),JSON.stringify({format:1,name:'Valid',region:'PAL',...invalid}));
   await assert.rejects(service.importMod(exported),/Invalid mod metadata/);assert.equal(service.state.mods.length,before);
  }
  await fs.writeFile(path.join(exported,'rage-mod.json'),JSON.stringify({format:1,name:'Invalid\nname',region:'PAL'}));
  await assert.rejects(service.importMod(exported),/Invalid mod metadata/);assert.equal(service.state.mods.length,before);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('failed game launch releases state and reports process failure on retry',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-launch-failure-'));
 try{
  const data=path.join(root,'data'),disc=path.join(root,'disc.bin');await fs.mkdir(data);await fs.writeFile(path.join(data,'manifest.json'),'{}');await fs.writeFile(disc,'fixture');
  let notify;const stopped=new Promise(resolve=>notify=resolve);
  const service=new LauncherService({root,bin:root,config:path.resolve(__dirname,'../resources/rage-port.ini'),onChange:s=>{if(!s.running&&s.error?.includes('exited with code'))notify(s);}});await service.init();service.state.disc={path:disc,data,region:'NTSC-U'};
  await assert.rejects(service.launch(),/ENOENT/);assert.equal(service.game,null);assert.equal(service.busy,null);
  // Node rejects the game's command-line flags and exits: an actual child
  // failure without depending on a platform-specific shell fixture.
  service.tool=()=>process.execPath;service.gameError='Previous game failure';
  await service.launch();
  // The child may already have exited while launch publishes its final state.
  // In either ordering the previous failure must be replaced, not retained.
  assert.notEqual(service.gameError,'Previous game failure');
  const snapshot=await Promise.race([stopped,new Promise((_,reject)=>{const timer=setTimeout(()=>reject(Error('Game close was not reported')),5000);timer.unref();})]);
  assert.match(snapshot.error,/game-process.log/);assert.equal(service.game,null);assert.equal(service.busy,null);
  assert.ok((await fs.readFile(path.join(root,'game-process.log'),'utf8')).length>0);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('diagnostics reads bounded tails and handles logs not created yet',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-log-tail-'));
 try{
  const service=new LauncherService({root});assert.equal((await service.logs()).every(log=>log.missing),true);
  await fs.writeFile(path.join(root,'game-process.log'),'x'.repeat(70000)+'last error');await fs.writeFile(path.join(root,'game.log'),'renderer ready');
  const logs=await service.logs();assert.equal(logs[0].text.length,65536);assert.equal(logs[0].truncated,true);assert.ok(logs[0].text.endsWith('last error'));assert.equal(logs[1].text,'renderer ready');assert.equal(logs[1].truncated,false);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
