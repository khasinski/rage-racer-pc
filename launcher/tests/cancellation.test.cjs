const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {run}=require('../main/service.cjs');
const {LauncherService}=require('../main/service.cjs');

test('cancellation waits for child exit and remains cancellation after graceful exit',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'rage-cancel-'));
 const marker=path.join(dir,'stopped');const controller=new AbortController();
 let ready;const started=new Promise(resolve=>{ready=resolve;});
 const command=`const fs=require('fs');process.on('SIGTERM',()=>setTimeout(()=>{fs.writeFileSync(process.argv[1],'stopped');process.exit(0);},80));process.stdout.write('ready');setInterval(()=>{},1000);`;
 try{
  const task=run(process.execPath,['-e',command,marker],{signal:controller.signal,onLog:text=>{if(text.includes('ready'))ready();}});
  // Attach the rejection handler before issuing cancellation.
  const rejected=assert.rejects(task,/Operation canceled/);
  await started;controller.abort();await rejected;
  if(process.platform!=='win32')assert.equal(await fs.readFile(marker,'utf8'),'stopped');
 }finally{await fs.rm(dir,{recursive:true,force:true});}
});

test('canceling mod import does not install a partial mod or retain the job lock',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-cancel-mod-'));
 try{
  const source=path.join(root,'source');await fs.mkdir(path.join(source,'raw'),{recursive:true});
  await fs.writeFile(path.join(source,'raw','asset_010.bin'),'fixture');
  let service;
  service=new LauncherService({root:path.join(root,'profile'),bin:path.resolve(__dirname,'../resources/bin'),config:path.resolve(__dirname,'../resources/rage-port.ini'),onChange:state=>{if(state.busy==='Importing mod'&&state.canCancel)service.cancel();}});
  await service.init();service.state.disc={region:'NTSC-U'};
  await assert.rejects(service.importMod(source),/Operation canceled/);
  assert.equal(service.state.mods.length,0);assert.equal(service.busy,null);
  assert.deepEqual(await fs.readdir(path.join(service.root,'mods')),[]);
  assert.equal(await fs.readFile(path.join(source,'raw','asset_010.bin'),'utf8'),'fixture');
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('cancellation terminates a child that ignores SIGTERM before settling',
 {skip:process.platform==='win32',timeout:15000},async()=>{
 const controller=new AbortController();let ready,pid;
 const started=new Promise(resolve=>{ready=resolve;});
 const task=run(process.execPath,['-e',"process.on('SIGTERM',()=>{});process.stdout.write(String(process.pid)+'\\n');setInterval(()=>{},1000);"],
  {signal:controller.signal,onLog:text=>{pid=Number(text.trim());ready();}});
 const rejected=assert.rejects(task,/Operation canceled/);
 await started;assert.ok(Number.isInteger(pid)&&pid>0);
 controller.abort();await rejected;
 assert.throws(()=>process.kill(pid,0),{code:'ESRCH'});
});

test('asset replacement holds one job lock and cancellation leaves no replacement',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-replace-cancel-'));
 try{
  const source=path.join(root,'asset.bin');await fs.writeFile(source,'replacement bytes');
  let service,shouldCancel=true;
  service=new LauncherService({root:path.join(root,'profile'),bin:path.resolve(__dirname,'../resources/bin'),config:path.resolve(__dirname,'../resources/rage-port.ini'),onChange:state=>{if(shouldCancel&&state.busy==='Importing asset replacement'&&state.canCancel)service.cancel();}});
  await service.init();service.state.disc={region:'NTSC-U'};
  await assert.rejects(service.replaceAsset(10,source),/Operation canceled/);
  assert.equal(service.state.mods.length,0);assert.equal(service.busy,null);
  assert.equal((await fs.readdir(service.root)).some(n=>n.startsWith('asset-replacement-')),false);
  shouldCancel=false;
  service.game={};await assert.rejects(service.replaceAsset(10,source),/running/);service.game=null;
  const id=await service.replaceAsset(10,source);
  const saved=JSON.parse(await fs.readFile(path.join(service.root,'launcher.json')));
  assert.equal(saved.mods.length,1);assert.equal(saved.mods[0].id,id);
  assert.equal(saved.mods[0].name,'Asset 10 replacement');assert.equal(saved.mods[0].enabled,false);
  assert.equal(await fs.readFile(path.join(service.root,'mods',id,'raw/asset_010.bin'),'utf8'),'replacement bytes');
  assert.equal(await fs.readFile(source,'utf8'),'replacement bytes');
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('material cancellation preserves the manifest and blocks concurrent changes',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-material-cancel-'));
 try{
  const source=path.join(root,'source');await fs.mkdir(source);
  const original='[materials]\n"car.0.material.0" = "lit opaque 0.3 0 1 1 1 1 0 0 0"\n';
  await fs.writeFile(path.join(source,'mod.toml'),original);
  let service,cancel=false,competing;
  service=new LauncherService({root:path.join(root,'profile'),bin:path.resolve(__dirname,'../resources/bin'),config:path.resolve(__dirname,'../resources/rage-port.ini'),onChange:state=>{
   if(cancel&&state.busy==='Saving material'&&state.canCancel){
    competing=assert.rejects(service.toggleMod(service.state.mods[0].id,true),/current operation/);
    service.cancel();
   }
  }});
  await service.init();service.state.disc={region:'NTSC-U'};await service.importMod(source);
  const mod=service.state.mods[0];cancel=true;
  await assert.rejects(service.saveMaterial(mod.id,'car.0.material.0','lit opaque 0.8 0 1 1 1 1 0 0 0'),/Operation canceled/);
  await competing;
  assert.equal(await fs.readFile(path.join(service.root,'mods',mod.id,'mod.toml'),'utf8'),original);
  assert.equal(mod.manifest.materials['car.0.material.0'],'lit opaque 0.3 0 1 1 1 1 0 0 0');
  assert.equal(service.busy,null);assert.equal(mod.enabled,false);
  assert.equal((await fs.readdir(service.root)).some(n=>n.startsWith('material-')),false);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('canceling mod export removes partial output and preserves installed files',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-export-cancel-'));
 try{
  const source=path.join(root,'source');await fs.mkdir(path.join(source,'raw'),{recursive:true});
  await fs.writeFile(path.join(source,'raw/asset_010.bin'),'original');
  let service;
  service=new LauncherService({root:path.join(root,'profile'),bin:path.resolve(__dirname,'../resources/bin'),config:path.resolve(__dirname,'../resources/rage-port.ini'),onChange:state=>{if(state.busy==='Exporting mod')service.cancel();}});
  await service.init();service.state.disc={region:'NTSC-U'};await service.importMod(source);const id=service.state.mods[0].id;
  await assert.rejects(service.exportMod(id,root),/Operation canceled/);
  await assert.rejects(fs.access(path.join(root,'rage-mod-'+id)));
  assert.equal(await fs.readFile(path.join(service.root,'mods',id,'raw/asset_010.bin'),'utf8'),'original');
  assert.equal(service.busy,null);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('native output preserves split UTF-8 and rejects overflow after child termination',async()=>{
 const unicode=await run(process.execPath,['-e',"process.stdout.write(Buffer.from([0xe2]));setTimeout(()=>process.stdout.write(Buffer.from([0x82,0xac])),20)"],{maxOutput:3});
 assert.equal(unicode,'€');
 await assert.rejects(run(process.execPath,['-e',"process.stdout.write('123456789');setInterval(()=>{},1000)"],{maxOutput:8}),/output exceeds the 8-byte limit/);
 assert.equal(await run(process.execPath,['-e',"process.stdout.write('12345678')"],{maxOutput:8}),'12345678');
});
