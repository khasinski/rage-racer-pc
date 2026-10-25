const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),os=require('node:os'),path=require('node:path');
const {LauncherService,run}=require('../main/service.cjs');
const {activeProfile}=require('../main/project-build.cjs');
const repo=path.resolve(__dirname,'../..');
test('material edits use project history and native validation without changing the library package',async t=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-project-material-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));
 const archive=path.join(root,'RAGE.BIN'),config=path.join(root,'default.ini');await fs.writeFile(archive,'material source');await fs.writeFile(config,'[video]\nrenderer = modern\n');
 const s=new LauncherService({root,bin:path.resolve(__dirname,'../resources/bin'),config,modsRoot:path.join(root,'game-mods')});await s.init();s.requireReady=()=>{};s.state.disc={archive,region:'PAL',path:archive,data:root};
 const original={id:'library',enabled:false,files:[],manifest:{materials:{'car.0.material.0':'lit opaque 0.35 0 1 1 1 1 0 0 0'}}};s.state.mods=[original];
 const before=JSON.stringify(original),key='car.0.material.0',properties='lit opaque 0.7 0.2 0.8 0.6 0.4 1 0 0 0';
 let p=await s.project();p=await s.saveMaterial('library',key,properties,p.revision);
 assert.equal(JSON.stringify(original),before);assert.equal(p.changes[0].field,'material');
 await assert.rejects(s.saveMaterial('library',key,properties,p.revision-1),/Project changed/);
 await assert.rejects(s.saveMaterial('library',key,'lit opaque 9 0 1 1 1 1 0 0 0',p.revision));
 assert.equal((await s.project()).revision,p.revision);
 p=await s.projectHistory('undo',p.revision);assert.equal(p.changes.length,0);
 p=await s.projectHistory('redo',p.revision);assert.equal(p.changes.length,1);
 await s.applyProject(p.revision);
 const pointer=JSON.parse(await fs.readFile(path.join(s.modsRoot,'active.json')));
 const manifest=JSON.parse(await run(s.tool('rage-mod-cli'),[path.join(s.modsRoot,'builds',pointer.build,'mod.toml')]));
 assert.equal(manifest.materials[key],p.changes[0].value);assert.equal(JSON.stringify(original),before);
});
test('imported project model participates in history and is frozen in the applied build',async t=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-project-model-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));
 const archive=path.join(root,'RAGE.BIN'),config=path.join(root,'default.ini'),input=path.join(root,'model.obj');
 await fs.writeFile(archive,'model source');await fs.writeFile(config,'[video]\nrenderer = modern\n');
 await fs.writeFile(input,'v 0 0 0\nv 1 0 0\nv 0 1 0\n vt 0 0\nvn 0 0 1\nusemtl rage_65535\nf 1/1/1 2/1/1 3/1/1\n');
 const s=new LauncherService({root:path.join(root,'editor'),bin:path.resolve(__dirname,'../resources/bin'),config,modsRoot:path.join(root,'mods')});
 await s.init();s.requireReady=()=>{};s.state.disc={archive,region:'PAL',path:archive,data:root};
 const key='car.player.10.part.0';
 s.carTool=async args=>JSON.stringify(args.some(a=>a.startsWith('tools.car_export='))?{materials:[]}:{parts:[{key,name:'Esperanza',part:0}]});
 let p=await s.importCar(key,input);assert.equal(s.state.mods.length,0);assert.equal(p.changes[0].id,key);
 const reference=p.changes[0].value;
 p=await s.projectHistory('undo',p.revision);assert.equal(p.changes.length,0);
 p=await s.projectHistory('redo',p.revision);assert.equal(p.changes[0].value,reference);
 p=await s.applyProject(p.revision);
 const pointer=await fs.readFile(path.join(s.modsRoot,'active.json'),'utf8'),active=JSON.parse(pointer);
 const directory=path.join(s.modsRoot,'builds',active.build);
 const manifest=JSON.parse(await run(s.tool('rage-mod-cli'),[path.join(directory,'mod.toml')]));
 assert.equal(manifest.meshes[key],'meshes/project/'+reference.slice(7));
 const stored=path.join(path.dirname(s.editor.file),'files',reference.slice(7));
 await fs.chmod(stored,0o644);await fs.writeFile(stored,'damaged');
 await assert.rejects(s.applyProject(p.revision),/damaged/);
 assert.equal(await fs.readFile(path.join(s.modsRoot,'active.json'),'utf8'),pointer);
 assert.ok(await activeProfile(s));
});
test('apply compiles car deltas, atomically activates for launch, and retains the old build on failure',async()=>{
  const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-project-build-'));
  const rename=fs.rename,writeFile=fs.writeFile;
  try{
    const bin=path.join(root,'bin');await fs.mkdir(bin);
    const native='rage-mod-cli'+(process.platform==='win32'?'.exe':'');
    await fs.copyFile(path.resolve(__dirname,'../resources/bin',native),path.join(bin,native));
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
      await fs.copyFile(path.resolve(__dirname,'../resources/bin',exe),path.join(bin,exe));
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
    installed.enabled=false; // launcher cache must not override document selection
    await s.applyProject(p.revision);
    installed.enabled=true;
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
    await s.toggleMod(installed.id,false);
    await assert.rejects(s.applyProject(p.revision),/Project changed/);
    p=await s.project();await s.applyProject(p.revision);
    const next=path.dirname(await activeProfile(s));assert.notEqual(next,build);
    assert.deepEqual(JSON.parse(await fs.readFile(path.join(next,'files.json'))),[]);
    assert.deepEqual(await fs.readFile(mesh),original);
  }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('a source project saved without applying never composes its package draft at launch',async()=>{
  const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-project-save-only-launch-'));
  try{
    const archive=path.join(root,'RAGE.BIN');await fs.writeFile(archive,'source revision');
    await fs.writeFile(path.join(root,'manifest.json'),'{}');await fs.writeFile(path.join(root,'menu_music.wav'),'fixture');
    let notify;const stopped=new Promise(resolve=>notify=resolve);
    const s=new LauncherService({root,modsRoot:path.join(root,'game-mods'),bin:path.resolve(__dirname,'../resources/bin'),config:path.resolve(__dirname,'../resources/rage-port.ini'),onChange:state=>{if(!state.running&&state.error?.includes('exited with code'))notify();}});
    await s.init();s.state.disc={archive,path:archive,data:root,region:'PAL'};
    const folder=path.join(root,'package');await fs.mkdir(folder);
    await fs.writeFile(path.join(folder,'mod.toml'),'[materials]\n"car.0.material.0" = "lit opaque 0.3 0 1 1 1 1 0 0 0"\n');
    const mod=await s.importMod(folder);await s.toggleMod(mod.id,true);
    let p=await s.project();p=await s.editProject([{kind:'car',id:'car_00_g0',field:'price',value:0}],p.revision);
    await s.saveProject(p.revision);
    s.composeMods=async()=>{throw Error('Save-only draft must not become game content');};
    const tool=s.tool.bind(s);s.tool=name=>name==='rage-racer'?process.execPath:tool(name);
    await s.launch(); // Exercise the launch transport without opening a game.
    await Promise.race([stopped,new Promise((_,reject)=>{const timer=setTimeout(()=>reject(Error('Child did not stop')),5000);timer.unref();})]);
    assert.equal(await activeProfile(s),'');
    const config=await fs.readFile(path.join(root,'rage-port.ini'),'utf8');
    assert.ok(!config.includes('directory = '+s.modsRoot));
    assert.ok(!config.includes('profile = '+path.join(s.modsRoot,'builds')));
    const saved=JSON.parse(await fs.readFile(s.editor.file));
    assert.deepEqual(saved.selection.packages,[mod.id]);assert.equal(saved.changes[0].value,0);
    await assert.rejects(fs.access(path.join(s.modsRoot,'active.json')),{code:'ENOENT'});
  }finally{await fs.rm(root,{recursive:true,force:true});}
});
test('title menu PNG project edits publish semantic textures and preserve the previous build on bad layout',async t=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-title-project-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));
 const archive=path.join(root,'RAGE.BIN'),config=path.join(root,'default.ini');await fs.writeFile(archive,'title source');await fs.writeFile(config,'[video]\nrenderer=modern\n');
 const s=new LauncherService({root,bin:path.resolve(__dirname,'../resources/bin'),config,modsRoot:path.join(root,'mods')});await s.init();s.requireReady=()=>{};s.state.disc={archive,path:archive,region:'PAL',data:root};
 let p=await s.project();const {ProjectFiles}=require('../main/project-files.cjs');
 const bytes=Buffer.from('iVBORw0KGgoAAAANSUhEUgAAAHAAAAAQCAYAAADOFPsRAAAAHUlEQVR4nO3BAQ0AAADCoPdPbQ8HFAAAAAAAAPBqHBAAAXumE0gAAAAASUVORK5CYII=','base64');
 const input=path.join(root,'custom.png');await fs.writeFile(input,bytes);const reference=await new ProjectFiles(s.editor.file).put(input,'png');
 p=await s.editProject([{kind:'interface',id:'ui.title.custom',field:'texture',value:reference}],p.revision);
 p=await s.applyProject(p.revision);const pointerFile=path.join(s.modsRoot,'active.json'),pointer=await fs.readFile(pointerFile,'utf8');
 const directory=path.join(s.modsRoot,'builds',JSON.parse(pointer).build),manifest=JSON.parse(await run(s.tool('rage-mod-cli'),[path.join(directory,'mod.toml')]));
 assert.deepEqual(await fs.readFile(path.join(directory,manifest.textures['ui.title.custom'])),bytes);
 const small=Buffer.from('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVQIHWP4z8DwHwAFgAI/ScLbtAAAAABJRU5ErkJggg==','base64');
 await fs.writeFile(input,small);const invalid=await new ProjectFiles(s.editor.file).put(input,'png');
 p=await s.editProject([{kind:'interface',id:'ui.title.custom',field:'texture',value:invalid}],p.revision);
 await assert.rejects(s.applyProject(p.revision));assert.equal(await fs.readFile(pointerFile,'utf8'),pointer);
});
for(const key of ['track.big1.course.material.3','car.0.material.3','track.big1.model-bank-1.material.3'])test('project surface replacement overrides package variants and reset restores them: '+key,async t=>{
 const {ProjectFiles}=require('../main/project-files.cjs'),root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-track-variant-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));const archive=path.join(root,'RAGE.BIN'),config=path.join(root,'default.ini');await fs.writeFile(archive,'track texture source');await fs.writeFile(config,'[video]\nrenderer=modern\n');
 const s=new LauncherService({root:path.join(root,'editor'),bin:path.resolve(__dirname,'../resources/bin'),config,modsRoot:path.join(root,'mods')});await s.init();s.requireReady=()=>{};s.state.disc={archive,region:'PAL',path:archive,data:root};const folder=path.join(root,'package');await fs.mkdir(path.join(folder,'textures'),{recursive:true});const png=Buffer.from('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVQIHWP4z8DwHwAFgAI/ScLttAAAAABJRU5ErkJggg==','base64');await fs.writeFile(path.join(folder,'textures/original.png'),png);const other='track.big1.course.material.67',materialKind=key.startsWith('track.')?'track':'car';await fs.writeFile(path.join(folder,'mod.toml'),`[mod]\nid="track-texture"\n[textures]\n"${key}.variant.0"="textures/original.png"\n"${key}.variant.2"="textures/original.png"\n"${other}.variant.0"="textures/original.png"\n[materials]\n"${key}.variant.0"="lit opaque 0.4 0 1 1 1 1 0 0 0"\n"${key}.variant.2"="lit opaque 0.6 0 1 1 1 1 0 0 0"\n`);const mod=await s.importMod(folder);await s.toggleMod(mod.id,true);let p=await s.project();const file=path.join(root,'replacement.png');await fs.writeFile(file,png);const ref=await new ProjectFiles(s.editor.file).put(file,'png');p=await s.editProject([{kind:materialKind,id:key,field:'texture',value:ref}],p.revision);await s.applyProject(p.revision);
 async function manifest(){const pointer=JSON.parse(await fs.readFile(path.join(s.modsRoot,'active.json')));return JSON.parse(await run(s.tool('rage-mod-cli'),[path.join(s.modsRoot,'builds',pointer.build,'mod.toml')]));}
 const properties='lit opaque 0.8 0.2 0.5 0.6 0.7 1 0 0 0';p=await s.editProject([{kind:'car',id:key,field:'material',value:'lit opaque 0.3 0 1 1 1 1 0 0 0'}],p.revision);p=await s.saveMaterial('',key,properties,p.revision);assert.equal(p.changes.filter(c=>c.field==='material').length,1);assert.equal(p.changes.find(c=>c.field==='material').kind,materialKind);await s.applyProject(p.revision);let built=await manifest();assert.equal(built.materials[key],properties);assert.equal(built.materials[key+'.variant.0'],properties);assert.equal(built.materials[key+'.variant.2'],properties);assert.equal(built.textures[key],built.textures[key+'.variant.0']);assert.equal(built.textures[key],built.textures[key+'.variant.2']);assert.notEqual(built.textures[key],built.textures[other+'.variant.0']);p=await s.editProject([{kind:materialKind,id:key,field:'texture',value:null},{kind:materialKind,id:key,field:'material',value:null}],p.revision);await s.applyProject(p.revision);built=await manifest();assert.equal(built.materials[key],undefined);assert.equal(built.materials[key+'.variant.0'],'lit opaque 0.4 0 1 1 1 1 0 0 0');assert.equal(built.materials[key+'.variant.2'],'lit opaque 0.6 0 1 1 1 1 0 0 0');assert.equal(built.textures[key],undefined);assert.ok(built.textures[key+'.variant.0']);assert.equal(built.textures[key+'.variant.0'],built.textures[key+'.variant.2']);assert.equal(await fs.readFile(path.join(folder,'textures/original.png'),'base64'),png.toString('base64'));
});
test('fitment axle deltas read the fields in the order the editor saves them',()=>{
 const {axleDeltas}=require('../main/project-build.cjs');
 // [frontWidth,frontY,frontZ,frontScale,rearWidth,rearY,rearZ,rearScale,bodyY,bodyZ,bodyScale]
 assert.deepEqual(axleDeltas([8,40,60,100,16,10,20,100,0,0,100]),{dx:2,dy:30,dz:40,rearScalePct:100});
 // Body height and scale must not leak into the rear scale.
 assert.equal(axleDeltas([0,0,0,100,0,0,0,100,70,0,120]).rearScalePct,100);
 assert.equal(axleDeltas([0,0,0,100,0,0,0,80,0,0,100]).rearScalePct,80);
 // A body-only edit leaves the axle record alone.
 assert.deepEqual(axleDeltas([0,0,0,100,0,0,0,100,50,50,100]),{dx:0,dy:0,dz:0,rearScalePct:100});
});
