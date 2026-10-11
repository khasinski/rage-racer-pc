const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),os=require('node:os'),path=require('node:path');
const {LauncherService,run}=require('../main/service.cjs');
const {activeProfile}=require('../main/project-build.cjs');
const repo=path.resolve(__dirname,'../..');
test('apply compiles car deltas, atomically activates for launch, and retains the old build on failure',async()=>{
  const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-project-build-'));
  const rename=fs.rename,writeFile=fs.writeFile;
  try{
    const bin=path.join(root,'bin');await fs.mkdir(bin);
    const native='rage-mod-cli'+(process.platform==='win32'?'.exe':'');
    await fs.copyFile(path.join(repo,process.env.RAGE_LAUNCHER_BUILD_DIR||'build/release',native),path.join(bin,native));
    for(const file of ['cars.toml','cars.ntscj.toml'])await fs.copyFile(path.join(repo,file),path.join(bin,file));
    const archive=path.join(root,'RAGE.BIN'),config=path.join(root,'default.ini');
    await fs.writeFile(archive,'source revision one');await fs.writeFile(config,'[video]\nrenderer = modern\n');
    const s=new LauncherService({root,bin,config,modsRoot:path.join(root,'game-mods')});s.requireReady=()=>{};s.state.disc={archive,region:'PAL',path:archive,data:root};
    let p=await s.project();
    p=await s.editProject([{kind:'car',id:'car_00_g0',field:'price',value:0},{kind:'car',id:'car_00_g0',field:'manual_only',value:true}],p.revision);
    p=await s.applyProject(p.revision);assert.equal(p.dirty,false);
    const profile=await activeProfile(s),dir=s.modsRoot,pointer=path.join(dir,'active.json');
    const oldPointer=await fs.readFile(pointer,'utf8');
    const {cars}=JSON.parse(await run(s.tool('rage-mod-cli'),['--car-profile',profile]));
    assert.equal(cars[0].price,0);assert.equal(cars[0].manual_only,true);
    assert.equal(cars[0].shift_points.length,12);
    const configured=await s.configuration();
    assert.ok(configured.includes('directory = '+s.modsRoot));
    assert.ok(!configured.includes('profile = '+profile));
    // Save-only leaves the last applied build untouched.
    p=await s.editProject([{kind:'car',id:'car_00_g0',field:'price',value:123}],p.revision);
    await s.saveProject(p.revision);assert.equal(await activeProfile(s),profile);
    fs.rename=async(a,b)=>{if(b===pointer)throw Object.assign(Error('simulated pointer failure'),{code:'EIO'});return rename(a,b);};
    await assert.rejects(s.applyProject(p.revision),/pointer failure/);fs.rename=rename;
    assert.equal(await fs.readFile(pointer,'utf8'),oldPointer);assert.equal(await activeProfile(s),profile);
    assert.equal((await fs.readdir(path.join(dir,'builds'))).length,1);
    fs.writeFile=async(file,bytes,...rest)=>{
      if(path.basename(file)==='build.json')bytes=JSON.stringify({...JSON.parse(bytes),payloadHash:'0'.repeat(64)});
      return writeFile(file,bytes,...rest);
    };
    await assert.rejects(s.applyProject(p.revision),/payload.*damaged/);fs.writeFile=writeFile;
    assert.equal(await fs.readFile(pointer,'utf8'),oldPointer);assert.equal(await activeProfile(s),profile);
    assert.equal((await fs.readdir(path.join(dir,'builds'))).length,1);
    // All-or-nothing even when another screen introduces unsupported edits.
    p=await s.editProject([{kind:'track',id:'coast',field:'road',value:1}],p.revision);
    await assert.rejects(s.applyProject(p.revision),/not connected/);assert.equal(await activeProfile(s),profile);
    p=await s.editProject([{kind:'track',id:'coast',field:'road',value:null},{kind:'car',id:'car_00_g0',field:'price',value:-1}],p.revision);
    await assert.rejects(s.applyProject(p.revision),/metadata/);assert.equal(await activeProfile(s),profile);
    await fs.appendFile(profile,'\n# tampered\n');
    await assert.rejects(activeProfile(s),/damaged/);
    const baseline=await s.configuration();assert.ok(!baseline.includes('profile = '+profile));
    assert.match((await s.snapshot()).error,/Project activation failed.*damaged/s);
    await fs.writeFile(archive,'source revision two');
    await assert.rejects(s.applyProject(p.revision),/Source archive changed/);
  }finally{fs.rename=rename;fs.writeFile=writeFile;await fs.rm(root,{recursive:true,force:true});}
});

test('one applied build retains car edits and selected model packages, freezes bytes and rejects payload damage',async()=>{
  const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-project-payload-'));
  try{
    const bin=path.join(root,'bin');await fs.mkdir(bin);
    for(const name of ['rage-mod-cli','rage-mesh-obj']){
      const exe=name+(process.platform==='win32'?'.exe':'');
      await fs.copyFile(path.join(repo,process.env.RAGE_LAUNCHER_BUILD_DIR||'build/release',exe),path.join(bin,exe));
    }
    for(const file of ['cars.toml','cars.ntscj.toml'])await fs.copyFile(path.join(repo,file),path.join(bin,file));
    const config=path.join(root,'default.ini'),archive=path.join(root,'RAGE.BIN');
    await fs.writeFile(config,'[video]\nrenderer = modern\n');await fs.writeFile(archive,'source revision');
    const s=new LauncherService({root,bin,config,modsRoot:path.join(root,'game-mods')});
    s.requireReady=()=>{};s.state.disc={archive,path:archive,data:root,region:'PAL'};
    const mod=path.join(root,'package');await fs.mkdir(path.join(mod,'meshes'),{recursive:true});
    const obj=path.join(root,'body.obj');
    await fs.writeFile(obj,'v 0 0 0\nv 1 0 0\nv 0 1 0\nvt 0 0\nvn 0 0 1\nusemtl rage_0\nf 1/1/1 2/1/1 3/1/1\n');
    await run(s.tool('rage-mesh-obj'),['import',obj,path.join(mod,'meshes/body.rmesh')]);
    await fs.writeFile(path.join(mod,'mod.toml'),'[meshes]\n"car.player.10.part.0" = "meshes/body.rmesh"\n');
    await s.importMod(mod);const installed=s.state.mods[0];await s.toggleMod(installed.id,true);
    let p=await s.project();p=await s.editProject([{kind:'car',id:'car_00_g0',field:'price',value:0}],p.revision);
    await s.applyProject(p.revision);
    const profile=await activeProfile(s),build=path.dirname(profile);
    const manifest=JSON.parse(await run(s.tool('rage-mod-cli'),[path.join(build,'mod.toml')]));
    const mesh=path.join(build,manifest.meshes['car.player.10.part.0']);
    assert.ok(mesh);await run(s.tool('rage-mod-cli'),['--mesh',mesh]);
    assert.equal(JSON.parse(await run(s.tool('rage-mod-cli'),['--car-profile',profile])).cars[0].price,0);
    const pointer=JSON.parse(await fs.readFile(path.join(s.modsRoot,'active.json')));
    assert.equal(pointer.format,2);assert.equal(pointer.payloadHash.length,64);
    const inventory=JSON.parse(await fs.readFile(path.join(build,'files.json')));
    assert.ok(inventory.some(([name])=>name===manifest.meshes['car.player.10.part.0']));
    await fs.writeFile(path.join(root,'manifest.json'),'{}');
    await fs.writeFile(path.join(root,'menu_music.wav'),'fixture');
    const tool=s.tool.bind(s),compose=s.composeMods;
    s.tool=name=>name==='rage-racer'?process.execPath:tool(name);
    s.composeMods=async()=>{throw Error('Applied builds must not compose another tree at launch');};
    let notify;const stopped=new Promise(resolve=>notify=resolve);
    s.onChange=state=>{if(!state.running&&state.error?.includes('exited with code'))notify();};
    await s.launch(); // Node rejects game flags: actual child lifecycle, no game window.
    await Promise.race([stopped,new Promise((_,reject)=>{
      const timer=setTimeout(()=>reject(Error('Child did not stop')),5000);timer.unref();
    })]);
    s.tool=tool;s.composeMods=compose;s.onChange=()=>{};
    assert.equal(await activeProfile(s),profile);
    const configText=await fs.readFile(path.join(root,'rage-port.ini'),'utf8');
    assert.ok(configText.includes('directory = '+s.modsRoot));
    assert.ok(!configText.includes('profile = '+profile));
    // Installed packages are editable; applied builds remain frozen.
    const original=await fs.readFile(mesh);
    await fs.writeFile(path.join(s.root,'mods',installed.id,'meshes/body.rmesh'),'invalid mesh');
    assert.equal(await activeProfile(s),profile);assert.deepEqual(await fs.readFile(mesh),original);
    await assert.rejects(s.applyProject(p.revision));assert.equal(await activeProfile(s),profile);
    assert.equal((await fs.readdir(root)).some(n=>n.startsWith('active-mods-')||n.startsWith('mod-sources-')),false);
    await fs.appendFile(mesh,'changed');await assert.rejects(activeProfile(s),/payload.*damaged/);
    await fs.writeFile(mesh,original);
    await s.toggleMod(installed.id,false);await s.applyProject(p.revision);
    const next=path.dirname(await activeProfile(s));assert.notEqual(next,build);
    assert.deepEqual(JSON.parse(await fs.readFile(path.join(next,'files.json'))),[]);
    assert.deepEqual(await fs.readFile(mesh),original);
  }finally{await fs.rm(root,{recursive:true,force:true});}
});
