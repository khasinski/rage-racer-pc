const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),os=require('node:os'),path=require('node:path');
const {spawnSync}=require('node:child_process');
const {LauncherService}=require('../main/service.cjs');
const source=process.env.RR_EDITOR_GAME_SOURCE,disc=process.env.RR_EDITOR_GAME_DISC,game=process.env.RR_EDITOR_GAME_RELEASE;
test('normal game launch loads the published menu WAV through its PCM player',{
 skip:!source||!disc||!game,timeout:150000
},async t=>{
 const base=path.resolve(__dirname,'../..'),root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-audio-game-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));
 await fs.mkdir(path.join(root,'bu00'));await fs.writeFile(path.join(root,'bu00/.fixture'),'isolated portable storage');
 const executable=path.join(root,process.platform==='win32'?'game.exe':'game');await fs.copyFile(game,executable);await fs.chmod(executable,0o755);
 for(const name of await fs.readdir(path.dirname(game)))if(/\.(dll|dylib)$/i.test(name))await fs.copyFile(path.join(path.dirname(game),name),path.join(root,name));
 const s=new LauncherService({root:path.join(root,'editor'),bin:path.join(base,'launcher/resources/bin'),config:path.join(base,'rage-port.ini'),modsRoot:path.join(root,'mods')});await s.init();s.requireReady=()=>{};
 s.state.disc={archive:path.join(source,'RAGE.BIN'),path:disc,region:'PAL',data:path.join(source,'data')};const p=await s.project();
 // A trimmed, quieter replacement must reach the same normal-game player.
 let edited=await s.importAudio('music.course-select',path.join(source,'data/menu_music.wav'),p.revision);
 await s.editAudio('music.course-select',{start:44100,end:132300,gain:50},edited.revision);await s.publishMods();
 for(const renderer of ['classic','modern']){
 const args=['--config',path.join(base,'rage-port.ini')];
 for(const [key,value]of Object.entries({'disc.image':disc,'disc.choose':'false','modern.assets':'disc','mods.directory':s.modsRoot,'cars.profile':path.join(base,'cars.toml'),'audio.menu_music':path.join(source,'data/menu_music.wav'),'video.renderer':renderer,'video.modern':'false','input.disable_host':'true','race.enabled':'false','stop.scene':'5','stop.timer':'0','diagnostics.log':path.join(root,'game.log')}))args.push('--set',`${key}=${value}`);
 const result=spawnSync(executable,args,{cwd:root,env:{...process.env,PSYZ_HEADLESS:'1',SDL_AUDIODRIVER:'dummy',RAGE_PORT_TEST_MODE:'1'},windowsHide:true,timeout:60000,encoding:'utf8',maxBuffer:2*1024*1024});
 assert.ifError(result.error);const log=result.stdout+result.stderr+await fs.readFile(path.join(root,'game.log'),'utf8');assert.equal(result.status,0,log);
 assert.match(log,/loaded music\.course-select override: .*audio\/project\/.+\.wav/);
 assert.match(log,/stopping at scene 5 timer/);assert.doesNotMatch(log,/invalid music\.course-select override/);
 }
});
