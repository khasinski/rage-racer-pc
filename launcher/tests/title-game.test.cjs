const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),os=require('node:os'),path=require('node:path');
const {LauncherService}=require('../main/service.cjs');
const source=process.env.RR_EDITOR_GAME_SOURCE,disc=process.env.RR_EDITOR_GAME_DISC;
const game=process.env.RR_EDITOR_GAME_SMOKE;
// Licensed, imported assets stay outside the repository. This opt-in test uses
// a real native game; it does not substitute a renderer or successful build.
test('published title PNG changes only CUSTOM in both native renderers',{
 skip:!source||!disc||!game,timeout:240000
},async t=>{
 const base=path.resolve(__dirname,'../..'),root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-title-game-'));
 t.after(()=>fs.rm(root,{recursive:true,force:true}));
 await fs.mkdir(path.join(root,'bu00'));await fs.writeFile(path.join(root,'bu00/.fixture'),'isolated portable storage');
 const executable=path.join(root,process.platform==='win32'?'game.exe':'game');
 await fs.copyFile(game,executable);await fs.chmod(executable,0o755);
 // Copy companion runtime libraries on platforms that require them.
 for(const name of await fs.readdir(path.dirname(game)))if(/\.(dll|dylib)$/i.test(name))await fs.copyFile(path.join(path.dirname(game),name),path.join(root,name));
 const s=new LauncherService({root:path.join(root,'editor'),bin:path.join(base,'launcher/resources/bin'),config:path.join(base,'rage-port.ini'),modsRoot:path.join(root,'mods')});
 await s.init();s.requireReady=()=>{};
 s.state.disc={archive:path.join(source,'RAGE.BIN'),path:disc,region:'PAL',data:path.join(source,'data')};
 await s.project();const image=path.join(root,'red.png');await s.exportTitleImage('ui.title.custom.selected',image);
 async function capture(renderer,edited){
  const output=path.join(root,`${renderer}-${edited}.ppm`),args=['--config',path.join(base,'rage-port.ini')];
  const settings={'disc.image':disc,'disc.choose':'false','modern.assets':'disc','mods.directory':edited?s.modsRoot:'','cars.profile':path.join(base,'cars.toml'),'audio.menu_music':path.join(source,'data/menu_music.wav'),'video.renderer':renderer,'video.modern':'false','video.internal_scale':'1','video.aspect':'4:3','input.disable_host':'true','input.state_script':'4@90:START','run.frames':'1000','stop.scene':'4','stop.timer':'180','capture.path':output,'diagnostics.log':path.join(root,'game.log'),'race.enabled':'false'};
  for(const [key,value] of Object.entries(settings))args.push('--set',`${key}=${value}`);
  const {spawnSync}=require('node:child_process');
  const result=spawnSync(executable,args,{cwd:root,env:{...process.env,SDL_AUDIODRIVER:'dummy',PSYZ_HEADLESS:'1'},windowsHide:true,timeout:60000,encoding:'utf8',maxBuffer:2*1024*1024});
  assert.ifError(result.error);const log=result.stdout+result.stderr;assert.equal(result.status,0,log);
  assert.match(log,/scene 4, frontend 2/);
  const data=await fs.readFile(output),header=/^P6\s+(\d+)\s+(\d+)\s+255\s/.exec(data.subarray(0,100).toString('ascii'));
  assert.ok(header,'expected P6 native capture');assert.equal(+header[1],320);assert.equal(+header[2],240);
  return data.subarray(header[0].length);
 }
 const originals={};for(const renderer of ['classic','modern'])originals[renderer]=await capture(renderer,false);
 const p=await s.project();await s.importTitleImage('ui.title.custom',image,p.revision);await s.publishMods();
 for(const renderer of ['classic','modern']){
  const edited=await capture(renderer,true),original=originals[renderer];assert.equal(edited.length,320*240*3);
  let changed=0;
  for(let y=0;y<240;y++)for(let x=0;x<320;x++){
   const offset=(y*320+x)*3;
   if(!edited.subarray(offset,offset+3).equals(original.subarray(offset,offset+3))){
    assert.ok(x>=104&&x<216&&y>=148&&y<164,`${renderer}: unrelated pixel changed at ${x},${y}`);changed++;
   }
  }
  assert.ok(changed>500,`${renderer}: published image did not change the button`);
 }
});
