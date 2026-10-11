const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),os=require('node:os'),path=require('node:path'),{spawnSync}=require('node:child_process');
const {LauncherService}=require('../main/service.cjs');
const source=process.env.RR_EDITOR_GAME_SOURCE,disc=process.env.RR_EDITOR_GAME_DISC,game=process.env.RR_EDITOR_GAME_RELEASE;
test('normal race loads edited scenery and atmosphere without an asset_000 marker',{
 skip:!source||!disc||!game,timeout:150000
},async t=>{
 const base=path.resolve(__dirname,'../..'),root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-scenery-game-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));
 await fs.mkdir(path.join(root,'bu00'));await fs.writeFile(path.join(root,'bu00/.fixture'),'isolated portable storage');
 const executable=path.join(root,process.platform==='win32'?'game.exe':'game');await fs.copyFile(game,executable);await fs.chmod(executable,0o755);
 for(const name of await fs.readdir(path.dirname(game)))if(/\.(dll|dylib)$/i.test(name))await fs.copyFile(path.join(path.dirname(game),name),path.join(root,name));
 const s=new LauncherService({root:path.join(root,'editor'),bin:path.join(base,'launcher/resources/bin'),config:path.join(base,'rage-port.ini'),modsRoot:path.join(root,'mods')});await s.init();s.requireReady=()=>{};s.state.disc={archive:path.join(source,'RAGE.BIN'),path:disc,region:'PAL',data:path.join(source,'data')};
 const p=await s.project(),scene=await s.trackScene(88),index=scene.objects.findIndex(o=>o.model>=0);assert.ok(index>=0);let edited=await s.duplicateTrackObject(88,index,p.revision);assert.ok(scene.environment?.cues.length);
 await s.saveEnvironment(88,[...scene.environment.cues.map((cue,index)=>({index,values:{color_0:0xcc33ff,color_2:0xcc33ff}})),...scene.environment.palettes.map((palette,index)=>({type:'palette',index,values:Object.fromEntries(Array.from({length:15},(_,i)=>['color_'+(i+1),0x00ff00]))}))],edited.revision);await s.publishMods();
 const pointer=JSON.parse(await fs.readFile(path.join(s.modsRoot,'active.json'))),build=path.join(s.modsRoot,'builds',pointer.build);await assert.rejects(fs.stat(path.join(build,'raw/asset_000.bin')),/ENOENT/);
 const scenario=path.join(root,'race.ini');await fs.writeFile(scenario,'[race]\nmode=grand-prix\nclass=0\ncourse=0\ncar=3\n[boot]\ndirect=true\nskip_sequences=true\n');
 for(const renderer of ['classic','modern']){
  const capture=path.join(base,'build/reports/editor/atmosphere-palettes-modern.ppm');await fs.mkdir(path.dirname(capture),{recursive:true});
  const logFile=path.join(root,renderer+'.log'),args=['--config',path.join(base,'rage-port.ini'),'--scenario',scenario];
  for(const [key,value]of Object.entries({'disc.image':disc,'disc.choose':'false','modern.assets':'disc','mods.directory':s.modsRoot,'cars.profile':path.join(base,'cars.toml'),'audio.menu_music':path.join(source,'data/menu_music.wav'),'video.renderer':renderer,'video.internal_scale':'1','video.ray_tracing':'off','video.modern':'false','input.disable_host':'true','stop.scene':'12','stop.timer':'20','diagnostics.log':logFile}))args.push('--set',`${key}=${value}`);
  if(renderer==='modern'){await fs.rm(capture,{force:true});args.push('--set','capture.path='+capture);}
  const result=spawnSync(executable,args,{cwd:root,env:{...process.env,PSYZ_HEADLESS:'1',SDL_AUDIODRIVER:'dummy',RAGE_PORT_TEST_MODE:'1'},windowsHide:true,timeout:60000,encoding:'utf8',maxBuffer:2*1024*1024});assert.ifError(result.error);
  const log=result.stdout+result.stderr+await fs.readFile(logFile,'utf8');assert.equal(result.status,0,log);assert.match(log,/asset 88 overridden, \d+ -> \d+ bytes/);assert.match(log,/stopping at scene 12 timer/);assert.doesNotMatch(log,/override for asset 88 has invalid|invalid track.*pack/i);
  if(renderer==='modern'){const frame=await fs.readFile(capture),header=/^P6\s+(\d+)\s+(\d+)\s+255\s/.exec(frame.subarray(0,100).toString('ascii'));assert.ok(header);const width=Number(header[1]),height=Number(header[2]),pixels=frame.subarray(header[0].length);assert.equal(pixels.length,width*height*3);let green=0;for(let i=0;i<width*Math.floor(height/3)*3;i+=3)if(pixels[i+1]>50&&pixels[i+1]>pixels[i]*2&&pixels[i+1]>pixels[i+2]*2)green++;assert.ok(green>50,'changed sky palette did not reach the rendered sky');}
 }
});
