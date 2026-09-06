const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {run,LauncherService}=require('../main/service.cjs');
const bin=path.resolve(__dirname,'../resources/bin');
const tool=path.join(bin,'rage-mod-cli'+(process.platform==='win32'?'.exe':''));
const copy=pairs=>run(tool,['--copy-snapshot-stdin'],{input:Buffer.from(JSON.stringify(pairs))});
test('export freezes profile metadata, uses compiled copies and removes invalid exports',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-export-snapshot-'));
 try{
  const source=path.join(root,'source');await fs.mkdir(path.join(source,'raw'),{recursive:true});
  await fs.writeFile(path.join(source,'raw/asset_010.bin'),'original');
  const service=new LauncherService({root:path.join(root,'profile'),bin,config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();service.state.disc={region:'PAL'};
  const mod=await service.importMod(source);
  const snapshot=service.snapshotModFiles.bind(service);
  let copied=false;
  service.snapshotModFiles=async(...args)=>{
   await snapshot(...args);copied=true;
   mod.name='changed during export';
   await fs.writeFile(path.join(args[0],'raw/asset_010.bin'),'changed');
  };
  const exported=await service.exportMod(mod.id,root);
  assert.equal(copied,true);
  assert.equal(await fs.readFile(path.join(exported,'raw/asset_010.bin'),'utf8'),'original');
  assert.equal(JSON.parse(await fs.readFile(path.join(exported,'rage-mod.json'))).name,'source');
  // A pre-existing destination is never removed or overwritten.
  await assert.rejects(service.exportMod(mod.id,root),/EEXIST/);
  assert.equal(await fs.readFile(path.join(exported,'raw/asset_010.bin'),'utf8'),'original');
  await fs.rm(exported,{recursive:true});
  mod.name='';
  await assert.rejects(service.exportMod(mod.id,root),/Invalid mod metadata/);
  await assert.rejects(fs.access(exported));
  assert.equal(service.busy,null);
  mod.name='valid';
  service.snapshotModFiles=async(...args)=>{
   await snapshot(...args);throw Error('injected copy failure');
  };
  await assert.rejects(service.exportMod(mod.id,root),/injected copy failure/);
  await assert.rejects(fs.access(exported));
  assert.equal(await fs.readFile(path.join(service.root,'mods',mod.id,'raw/asset_010.bin'),'utf8'),'changed');
  assert.equal(service.busy,null);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
test('compiled snapshot copies exact bytes and refuses overwrite or malformed requests',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-snapshot-'));
 try{
  const source=path.join(root,'źródło.bin'),target=path.join(root,'snapshot.bin');
  const bytes=Buffer.alloc(150000);for(let i=0;i<bytes.length;i++)bytes[i]=i%251;
  await fs.writeFile(source,bytes);await copy([[source,target]]);
  assert.deepEqual(await fs.readFile(target),bytes);
  await assert.rejects(copy([[source,target]]),/snapshot/);
  assert.deepEqual(await fs.readFile(target),bytes);
  await assert.rejects(copy([[source,source]]),/snapshot/);
  assert.deepEqual(await fs.readFile(source),bytes);
  const absent=path.join(root,'absent');
  for(const pairs of [[[source,absent],['missing']],[[source,absent+'\0suffix']],[[source,42]],{}]){
   await assert.rejects(copy(pairs),/snapshot/);await assert.rejects(fs.access(absent));
  }
  await assert.rejects(copy([[path.join(root,'missing'),absent]]),/snapshot/);
  await assert.rejects(fs.access(absent));
  await copy([]);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
test('import validates and installs its private snapshot, not subsequently changed originals',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-import-snapshot-'));
 try{
  const source=path.join(root,'my-pack');await fs.mkdir(source);
  const text='[mod]\nid="original"\n[materials]\n"car.a"="lit opaque 0.5 0 1 1 1 1 0 0 0"';
  await fs.writeFile(path.join(source,'mod.toml'),text);
  const service=new LauncherService({root:path.join(root,'profile'),bin,config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();service.state.disc={region:'PAL'};
  const snapshot=service.snapshotModFiles.bind(service);
  service.snapshotModFiles=async(...args)=>{
   await snapshot(...args);
   await fs.writeFile(path.join(source,'mod.toml'),'[mod]\nschema_version=999');
  };
  const mod=await service.importMod(source);
  assert.equal(mod.name,'my-pack');assert.equal(mod.manifest.id,'original');
  assert.equal(await fs.readFile(path.join(service.root,'mods',mod.id,'mod.toml'),'utf8'),text);
  // Corrupt copied data before validation: reject without publishing a mod.
  await fs.writeFile(path.join(source,'mod.toml'),text);
  service.snapshotModFiles=async(...args)=>{
   await snapshot(...args);await fs.writeFile(path.join(args[1],'mod.toml'),'[mod]\nschema_version=999');
  };
  const before=(await fs.readdir(path.join(service.root,'mods'))).sort();
  await assert.rejects(service.importMod(source),/Invalid mod manifest/);
  assert.equal(service.state.mods.length,1);
  assert.deepEqual((await fs.readdir(path.join(service.root,'mods'))).sort(),before);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
