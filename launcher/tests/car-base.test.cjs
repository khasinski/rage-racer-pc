const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {LauncherService,run}=require('../main/service.cjs');
async function fixture(t,real){
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-car-base-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));const data=path.join(root,'data');await fs.mkdir(path.join(data,'raw'),{recursive:true});
 let archive;if(real){archive=path.join(real,'RAGE.BIN');for(const bank of [0,10,11])await fs.copyFile(path.join(real,'data/raw/asset_'+String(bank).padStart(3,'0')+'.bin'),path.join(data,'raw/asset_'+String(bank).padStart(3,'0')+'.bin'));}
 else{archive=path.join(root,'RAGE.BIN');await fs.writeFile(archive,'synthetic source');await fs.writeFile(path.join(data,'raw/asset_000.bin'),Buffer.alloc(32));const spec=Buffer.alloc(2048);[32,512,640,768,1024].forEach((v,i)=>spec.writeUInt32LE(v,i*4));[8000,1000,4,7000].forEach((v,i)=>spec.writeInt16LE(v,32+0x100+i*2));await fs.writeFile(path.join(data,'raw/asset_011.bin'),spec);const model=Buffer.alloc(2048);model[8]=1;model.writeUInt32LE(40,32);model.writeUInt32LE(168,36);await fs.writeFile(path.join(data,'raw/asset_010.bin'),model);}
 const s=new LauncherService({root:path.join(root,'editor'),bin:path.resolve(__dirname,'../resources/bin'),modsRoot:path.join(root,'mods'),config:path.resolve(__dirname,'../resources/rage-port.ini')});await s.init();s.requireReady=()=>{};s.state.disc={archive,path:archive,region:'PAL',data};await s.project();return {root,data,s};
}
async function exercise(t,real){
 const {root,data,s}=await fixture(t,real),initial=await s.carSpec(10),field=(spec,key)=>spec.fields.find(f=>f.key===key),beforeSpec=await fs.readFile(path.join(data,'raw/asset_011.bin')),beforeModel=await fs.readFile(path.join(data,'raw/asset_010.bin'));
 const folder=path.join(root,'package');await fs.mkdir(path.join(folder,'raw'),{recursive:true});await run(s.tool('rage-mod-cli'),['--car-spec','write',path.join(data,'raw/asset_011.bin'),path.join(folder,'raw/asset_011.bin'),'revLimit','9000','torqueCurve0','45678','downshift2','123','speedDragDivisor','2100']);await run(s.tool('rage-mod-cli'),['--car-transmission','write',path.join(data,'raw/asset_010.bin'),path.join(folder,'raw/asset_010.bin'),'1']);await fs.writeFile(path.join(folder,'mod.toml'),'[mod]\nid="raw-spec"\n');const mod=await s.importMod(folder);await s.toggleMod(mod.id,true);
 let spec=await s.carSpec(10);for(const [key,value]of [['revLimit',9000],['torqueCurve0',45678],['downshift2',123],['speedDragDivisor',2100],['manualOnly',1]]){assert.equal(field(spec,key).value,value,key);assert.equal(field(spec,key).baseValue,value,key);assert.equal(field(spec,key).origin,'selected packages',key);}
 assert.equal(field(spec,'automaticAccelerationScale').value,field(initial,'automaticAccelerationScale').value);assert.equal(field(spec,'automaticAccelerationScale').origin,'built-in profile');
 let p=await s.saveCarSpec(10,{revLimit:9500,torqueCurve1:77777,upshift2:1234},spec.revision);spec=await s.carSpec(10);assert.equal(field(spec,'revLimit').value,9500);assert.equal(field(spec,'revLimit').baseValue,9000);assert.equal(field(spec,'revLimit').origin,'project');assert.equal(field(spec,'torqueCurve0').origin,'selected packages');assert.equal(field(spec,'torqueCurve1').origin,'project');assert.equal(field(spec,'torqueCurve2').origin,'built-in profile');await s.publishMods();
 const active=JSON.parse(await run(s.tool('rage-mod-cli'),['--active-build',s.modsRoot,p.source.region,p.source.fingerprint]));const cars=JSON.parse(await run(s.tool('rage-mod-cli'),['--car-profile',path.join(active.directory,'cars.toml')])).cars,car=cars.find(c=>c.id===spec.id);assert.equal(car.rev_limit,9500);assert.equal(car.speed_drag_divisor,2100);assert.equal(car.torque_curve[0],45678);assert.equal(car.torque_curve[1],77777);assert.equal(car.shift_points[2],123);assert.equal(car.shift_points[3],1234);assert.equal(car.manual_only,true);
 const disc=process.env.RR_EDITOR_GAME_DISC,game=process.env.RR_EDITOR_GAME_SMOKE;
 if(real&&disc&&game){
  const {spawnSync}=require('node:child_process'),base=path.resolve(__dirname,'../..'),exe=path.join(root,process.platform==='win32'?'game.exe':'game');
  await fs.mkdir(path.join(root,'bu00'),{recursive:true});await fs.writeFile(path.join(root,'bu00/.fixture'),'isolated');await fs.copyFile(game,exe);await fs.chmod(exe,0o755);
  for(const n of await fs.readdir(path.dirname(game)))if(/\.(dll|dylib)$/i.test(n))await fs.copyFile(path.join(path.dirname(game),n),path.join(root,n));
  const scenario=path.join(root,'race.ini');await fs.writeFile(scenario,'[race]\nmode=grand-prix\nclass=0\ncourse=0\ncar=0\n[boot]\ndirect=true\nskip_sequences=true\n');
  const boot=mods=>{
   const args=['--config',path.join(base,'rage-port.ini'),'--scenario',scenario],cfg={'disc.image':disc,'disc.choose':'false','modern.assets':'disc','mods.directory':mods,'cars.profile':'','audio.menu_music':path.join(real,'data/menu_music.wav'),'video.renderer':'classic','video.modern':'false','input.disable_host':'true','stop.scene':'12','stop.timer':'10','capture.path':path.join(root,'race.ppm')};
   for(const [k,v]of Object.entries(cfg))args.push('--set',`${k}=${v}`);
   const r=spawnSync(exe,args,{cwd:root,env:{...process.env,SDL_AUDIODRIVER:'dummy',PSYZ_HEADLESS:'1'},windowsHide:true,timeout:60000,encoding:'utf8',maxBuffer:2*1024*1024});
   assert.ifError(r.error);const log=r.stdout+r.stderr;assert.equal(r.status,0,log);return log;
  };
  assert.match(boot(''),/scene 12,.*manual=0/);assert.match(boot(s.modsRoot),/scene 12,.*manual=1/);
 }
 const pointer=await fs.readFile(path.join(s.modsRoot,'active.json')),packageSpec=path.join(s.root,'mods',mod.id,'raw/asset_011.bin'),valid=await fs.readFile(packageSpec),revision=(await s.project()).revision;await fs.writeFile(packageSpec,'broken raw spec');await assert.rejects(s.carSpec(10),/Invalid car specification/i);await assert.rejects(s.publishMods(),/Invalid car specification/i);assert.equal((await s.project()).revision,revision);assert.deepEqual(await fs.readFile(path.join(s.modsRoot,'active.json')),pointer);await fs.writeFile(packageSpec,valid);
 p=await s.saveCarSpec(10,{revLimit:9000},p.revision);assert.ok(!p.changes.some(c=>c.id===spec.id&&c.field==='rev_limit'));assert.equal(field(await s.carSpec(10),'revLimit').origin,'selected packages');await s.toggleMod(mod.id,false);assert.equal(field(await s.carSpec(10),'revLimit').value,field(initial,'revLimit').value);
 assert.deepEqual(await fs.readFile(path.join(data,'raw/asset_011.bin')),beforeSpec);assert.deepEqual(await fs.readFile(path.join(data,'raw/asset_010.bin')),beforeModel);
}
test('raw parameter deltas feed UI/build, project edits win, and reset restores the selected raw base',t=>exercise(t));
const real=process.env.RR_EDITOR_GAME_SOURCE;
test('actual PAL specification and transmission raw overrides survive the generated runtime profile',{skip:!real,timeout:120000},t=>exercise(t,real));
