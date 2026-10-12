const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),os=require('node:os'),path=require('node:path'),{spawnSync}=require('node:child_process');
const {LauncherService}=require('../main/service.cjs');
const source=process.env.RR_EDITOR_GAME_SOURCE,disc=process.env.RR_EDITOR_GAME_DISC,game=process.env.RR_EDITOR_GAME_RELEASE;
test('normal modern race loads all car parts and embedded PNGs from GLB with recovered mappings',{
 skip:!source||!disc||!game,timeout:150000
},async t=>{
 const base=path.resolve(__dirname,'../..'),root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-scenery-game-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));
 await fs.mkdir(path.join(root,'bu00'));await fs.writeFile(path.join(root,'bu00/.fixture'),'isolated portable storage');
 const executable=path.join(root,process.platform==='win32'?'game.exe':'game');await fs.copyFile(game,executable);await fs.chmod(executable,0o755);
 for(const name of await fs.readdir(path.dirname(game)))if(/\.(dll|dylib)$/i.test(name))await fs.copyFile(path.join(path.dirname(game),name),path.join(root,name));
 const s=new LauncherService({root:path.join(root,'editor'),bin:path.join(base,'launcher/resources/bin'),config:path.join(base,'rage-port.ini'),modsRoot:path.join(root,'mods')});await s.init();s.requireReady=()=>{};s.state.disc={archive:path.join(source,'RAGE.BIN'),path:disc,region:'PAL',data:path.join(source,'data')};
 const key='car.player.10.part.0';await s.project();const before=await s.previewWholeCar(10),exported=await s.exportCarSet(key,root);const filename=path.join(exported,'car.glb'),bytes=await fs.readFile(filename),oldLength=bytes.readUInt32LE(12),doc=JSON.parse(bytes.subarray(20,20+oldLength).toString());assert.equal(doc.meshes.length,3);assert.ok(doc.images.length);
 const png=Buffer.from('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVQIHWP4z8DwHwAFgAI/ScLttAAAAABJRU5ErkJggg==','base64'),bin=Buffer.from(bytes.subarray(20+oldLength));for(const image of doc.images){const view=doc.bufferViews[image.bufferView];assert.ok(view.byteLength>=png.length);png.copy(bin,8+view.byteOffset);view.byteLength=png.length;}
 const json=Buffer.from(JSON.stringify(doc)),padded=Buffer.alloc((json.length+3)&~3,32);json.copy(padded);const changed=Buffer.alloc(20+padded.length+bin.length);bytes.copy(changed,0,0,12);changed.writeUInt32LE(changed.length,8);changed.writeUInt32LE(padded.length,12);changed.write('JSON',16);padded.copy(changed,20);bin.copy(changed,20+padded.length);await fs.writeFile(filename,changed);
 const imported=await s.importCarSet(key,filename);assert.equal(imported.changes.filter(c=>c.field==='mesh').length,3);
 await s.projectHistory('undo',imported.revision);
 const lost=structuredClone(doc);delete lost.extras;for(const n of lost.nodes)delete n.extras;for(const m of lost.meshes)delete m.extras;for(const m of lost.materials){delete m.extras;m.name='Edited material';}
 const lostJson=Buffer.from(JSON.stringify(lost)),lostPad=Buffer.alloc((lostJson.length+3)&~3,32);lostJson.copy(lostPad);const lostFile=Buffer.alloc(20+lostPad.length+bin.length);bytes.copy(lostFile,0,0,12);lostFile.writeUInt32LE(lostFile.length,8);lostFile.writeUInt32LE(lostPad.length,12);lostFile.write('JSON',16);lostPad.copy(lostFile,20);bin.copy(lostFile,20+lostPad.length);await fs.writeFile(filename,lostFile);
 const mapping=await s.prepareCarImport(key,filename);const recovered=await s.completeCarImport(mapping.token,{nodes:mapping.nodes.map(n=>({node:n.node,instance:doc.nodes[n.node].extras.rr_instance})),materials:mapping.materials.map(m=>({material:m.material,word:doc.materials[m.material].extras.rr_material}))});assert.equal(recovered.changes.filter(c=>c.field==='mesh').length,3);
 const after=await s.previewWholeCar(10);assert.equal(after.indices.length,before.indices.length);
 for(let i=0;i<before.indices.length;i++)for(let c=0;c<9;c++)assert.ok(Math.abs(before.vertices[before.indices[i]*9+c]-after.vertices[after.indices[i]*9+c])<1e-5,`corner ${i} field ${c}`);
 await s.publishMods();
 const scenario=path.join(root,'race.ini');await fs.writeFile(scenario,'[race]\nmode=grand-prix\nclass=0\ncourse=0\ncar=0\n[boot]\ndirect=true\nskip_sequences=true\n');
 const renderer='modern';
  const logFile=path.join(root,renderer+'.log'),args=['--config',path.join(base,'rage-port.ini'),'--scenario',scenario];
  for(const [key,value]of Object.entries({'disc.image':disc,'disc.choose':'false','modern.assets':'disc','mods.directory':s.modsRoot,'cars.profile':path.join(base,'cars.toml'),'audio.menu_music':path.join(source,'data/menu_music.wav'),'video.renderer':renderer,'video.internal_scale':'1','video.ray_tracing':'off','video.modern':'false','modern.authored_cars':'0','input.disable_host':'true','stop.scene':'12','stop.timer':'20','diagnostics.log':logFile}))args.push('--set',`${key}=${value}`);

  const result=spawnSync(executable,args,{cwd:root,env:{...process.env,PSYZ_HEADLESS:'1',SDL_AUDIODRIVER:'dummy',RAGE_PORT_TEST_MODE:'1'},windowsHide:true,timeout:60000,encoding:'utf8',maxBuffer:2*1024*1024});assert.ifError(result.error);
  const log=result.stdout+result.stderr+await fs.readFile(logFile,'utf8');assert.equal(result.status,0,log);for(const part of [0,2,3])assert.ok(log.includes(`car mesh override car.player.10.part.${part} <-`),log);assert.match(log,/native texture override car\.0\.material\.\d+ <-.*\(1x1\)/);assert.match(log,/stopping at scene 12 timer/);assert.doesNotMatch(log,/invalid car mesh override|replace failed/);
});
