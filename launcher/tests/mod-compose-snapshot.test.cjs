const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {LauncherService,run}=require('../main/service.cjs');
const bin=path.resolve(__dirname,'../resources/bin');
const material=(id,color)=>`[mod]\nid="${id}"\n[materials]\n"car.a"="lit opaque 0.5 0 ${color} 1 0 0 0"`;
test('composition copy rejects a substituted staged symlink before publication',{skip:process.platform==='win32'},async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-compose-copy-link-'));
 try{
  const service=new LauncherService({root:path.join(root,'profile'),bin,config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();service.state.disc={region:'PAL'};
  const source=path.join(root,'source');await fs.mkdir(path.join(source,'raw'),{recursive:true});
  await fs.writeFile(path.join(source,'raw/asset_000.bin'),'private marker');
  const mod=await service.importMod(source);await service.toggleMod(mod.id,true);
  const snapshot=service.snapshotModFiles.bind(service);
  service.snapshotModFiles=async(...args)=>{
   await snapshot(...args);
   const staged=path.join(args[1],'raw/asset_000.bin');
   await fs.unlink(staged);await fs.symlink(path.join(source,'raw/asset_000.bin'),staged);
  };
  await assert.rejects(service.composeMods(),/Cannot snapshot mod files/);
  assert.equal(await fs.readFile(path.join(source,'raw/asset_000.bin'),'utf8'),'private marker');
  assert.deepEqual((await fs.readdir(service.root)).filter(n=>n.startsWith('mod-sources-')||n.startsWith('active-mods-')),[]);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
test('composition publishes a validated private tree and cleans up failed publication',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-compose-publish-'));
 const rename=fs.rename;
 try{
  const service=new LauncherService({root:path.join(root,'profile'),bin,config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();service.state.disc={region:'PAL'};
  const source=path.join(root,'source');await fs.mkdir(source);
  await fs.writeFile(path.join(source,'mod.toml'),material('base','1 0 0'));
  const mod=await service.importMod(source);await service.toggleMod(mod.id,true);
  let publications=0;
  fs.rename=async(from,to)=>{
   if(path.basename(to).startsWith('active-mods-')){
    publications++;
    assert.ok(path.basename(path.dirname(from)).startsWith('mod-sources-'));
    assert.deepEqual((await fs.readdir(service.root)).filter(n=>n.startsWith('active-mods-')),[]);
    const parsed=JSON.parse(await run(service.tool('rage-mod-cli'),[path.join(from,'mod.toml')]));
    assert.equal(parsed.materials['car.a'],'lit opaque 0.5 0 1 0 0 1 0 0 0');
    throw Error('injected publication failure');
   }
   return rename(from,to);
  };
  await assert.rejects(service.composeMods(),/injected publication failure/);
  assert.equal(publications,1);
  assert.deepEqual((await fs.readdir(service.root)).filter(n=>n.startsWith('mod-sources-')||n.startsWith('active-mods-')),[]);
  fs.rename=rename;
  assert.ok(path.basename(await service.composeMods()).startsWith('active-mods-'));
 }finally{fs.rename=rename;await fs.rm(root,{recursive:true,force:true});}
});
test('combined manifest overflow cleans failed output, preserves prior profile and allows retry',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-compose-capacity-'));
 try{
  const service=new LauncherService({root:path.join(root,'profile'),bin,config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();service.state.disc={region:'PAL'};
  const mods=[];
  for(const id of ['a','b']){
   const folder=path.join(root,id);await fs.mkdir(folder);
   let text=`[mod]\nid="${id}"\n[materials]\n`;
   for(let i=0;i<300;i++)text+=`"car.${id}.${i}"="lit opaque 0.5 0 1 1 1 1 0 0 0"\n`;
   await fs.writeFile(path.join(folder,'mod.toml'),text);
   mods.push(await service.importMod(folder));
  }
  await service.toggleMod(mods[0].id,true);
  const previous=await service.composeMods();
  const previousBytes=await fs.readFile(path.join(previous,'mod.toml'));
  await service.toggleMod(mods[1].id,true);
  assert.deepEqual(service.conflicts(),[],'distinct keys exceed capacity without conflicts');
  await assert.rejects(service.composeMods(),/Invalid mod manifest/);
  assert.deepEqual(await fs.readFile(path.join(previous,'mod.toml')),previousBytes);
  assert.deepEqual((await fs.readdir(service.root)).filter(n=>n.startsWith('active-mods-')),[path.basename(previous)]);
  assert.deepEqual((await fs.readdir(service.root)).filter(n=>n.startsWith('mod-sources-')),[]);
  await service.toggleMod(mods[1].id,false);
  const retry=await service.composeMods();
  assert.notEqual(retry,previous);
  assert.deepEqual(await fs.readFile(path.join(retry,'mod.toml')),previousBytes);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
test('raw overrides and original archive marker are copied from staging',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-compose-raw-'));
 try{
  const source=path.join(root,'source'),disc=path.join(root,'disc');
  await fs.mkdir(path.join(source,'raw'),{recursive:true});await fs.mkdir(path.join(disc,'raw'),{recursive:true});
  await fs.writeFile(path.join(source,'raw/asset_010.bin'),'original override');
  await fs.writeFile(path.join(disc,'raw/asset_000.bin'),'original marker');
  const service=new LauncherService({root:path.join(root,'profile'),bin,config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();service.state.disc={region:'PAL',data:disc};
  const mod=await service.importMod(source);await service.toggleMod(mod.id,true);
  const snapshot=service.snapshotModFiles.bind(service);
  service.snapshotModFiles=async(...args)=>{
   await snapshot(...args);
   const relative=args[0]===disc?'raw/asset_000.bin':'raw/asset_010.bin';
   await fs.writeFile(path.join(args[0],relative),'changed after snapshot');
  };
  const output=await service.composeMods();
  assert.equal(await fs.readFile(path.join(output,'raw/asset_010.bin'),'utf8'),'original override');
  assert.equal(await fs.readFile(path.join(output,'raw/asset_000.bin'),'utf8'),'original marker');
  assert.equal((await fs.readdir(service.root)).filter(n=>n.startsWith('mod-sources-')).length,0);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
test('composition freezes source bytes and keeps profile views separate',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-compose-snapshot-'));
 try{
  const service=new LauncherService({root:path.join(root,'profile'),bin,config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();service.state.disc={region:'PAL'};
  const source=path.join(root,'source');await fs.mkdir(source);
  await fs.writeFile(path.join(source,'mod.toml'),material('base','1 0 0'));
  const mod=await service.importMod(source);await service.toggleMod(mod.id,true);
  const installed=path.join(service.root,'mods',mod.id,'mod.toml');
  // Cached UI manifest deliberately differs from the installed source.
  await fs.writeFile(installed,material('base','0 1 0'));
  const snapshot=service.snapshotModFiles.bind(service);
  service.snapshotModFiles=async(...args)=>{
   await snapshot(...args);await fs.writeFile(installed,'[mod]\nschema_version=999');
  };
  const output=await service.composeMods();
  const result=JSON.parse(await run(service.tool('rage-mod-cli'),[path.join(output,'mod.toml')]));
  assert.equal(result.materials['car.a'],'lit opaque 0.5 0 0 1 0 1 0 0 0');
  assert.equal(mod.manifest.materials['car.a'],'lit opaque 0.5 0 1 0 0 1 0 0 0');
  assert.equal((await fs.readdir(service.root)).filter(n=>n.startsWith('mod-sources-')).length,0);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
test('fresh snapshot claims cannot bypass conflicts and failed staging is removed',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-compose-claims-'));
 try{
  const service=new LauncherService({root:path.join(root,'profile'),bin,config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();service.state.disc={region:'PAL'};
  for(const id of ['a','b']){
   const folder=path.join(root,id);await fs.mkdir(folder);
   await fs.writeFile(path.join(folder,'mod.toml'),id==='a'?material(id,'1 0 0'):'[mod]\nid="b"');
   const mod=await service.importMod(folder);await service.toggleMod(mod.id,true);
  }
  assert.equal(service.conflicts().length,0);
  const installed=path.join(service.root,'mods',service.state.mods[1].id,'mod.toml');
  await fs.writeFile(installed,material('b','0 1 0'));
  await assert.rejects(service.composeMods(),/conflict/);
  assert.equal(service.conflicts().length,0,'composition must not mutate cached UI state');
  const leftovers=(await fs.readdir(service.root)).filter(n=>n.startsWith('mod-sources-')||n.startsWith('active-mods-'));
  assert.deepEqual(leftovers,[]);
  await fs.writeFile(installed,'[textures]\n"car.b"="textures/missing.png"');
  await assert.rejects(service.composeMods(),/Missing texture/);
  assert.equal((await fs.readdir(service.root)).filter(n=>n.startsWith('mod-sources-')).length,0);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
