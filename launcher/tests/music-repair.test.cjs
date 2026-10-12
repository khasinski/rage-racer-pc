const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {LauncherService,run}=require('../main/service.cjs');
const repair=require('../main/menu-music.cjs');
async function fixture(t){
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-music-repair-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));
 const data=path.join(root,'data');await fs.mkdir(data);await fs.writeFile(path.join(data,'manifest.json'),'keep existing manifest');await fs.mkdir(path.join(data,'raw'));await fs.writeFile(path.join(data,'raw/asset_010.bin'),'keep raw mod');
 const s=new LauncherService({root,bin:path.resolve(__dirname,'../resources/bin'),modsRoot:path.join(root,'mods'),config:path.resolve(__dirname,'../resources/rage-port.ini')});await s.init();s.requireReady=()=>{};
 return {root,data,s};
}
test('cancelled or failed PCM repair leaves the library intact and removes extraction staging',async t=>{
 const {root,data,s}=await fixture(t),archive=path.join(root,'RAGE.BIN');await fs.writeFile(archive,'invalid archive');s.state.disc={data,archive,region:'PAL'};
 const controller=new AbortController();controller.abort();await assert.rejects(repair(s,controller.signal),/canceled/);
 await assert.rejects(repair(s),/exit|failure|archive|entries/i);
 await assert.rejects(fs.stat(path.join(data,'menu_music.wav')),/ENOENT/);
 assert.equal(await fs.readFile(path.join(data,'manifest.json'),'utf8'),'keep existing manifest');assert.equal(await fs.readFile(path.join(data,'raw/asset_010.bin'),'utf8'),'keep raw mod');assert.deepEqual((await fs.readdir(root)).filter(n=>n.startsWith('music-repair-')),[]);
});
const source=process.env.RR_EDITOR_GAME_SOURCE;
test('audio editor regenerates actual regional PCM and only publishes the missing generated file',{skip:!source,timeout:60000},async t=>{
 const {root,data,s}=await fixture(t),archive=path.join(source,'RAGE.BIN');s.state.disc={data,archive,path:archive,region:'PAL'};await s.project();
 const first=await s.audio(),cue=first.cues[0],expected=await fs.readFile(path.join(source,'data/menu_music.wav'));assert.equal(cue.origin,'disc');assert.deepEqual(Buffer.from(cue.wav,'base64'),expected);
 assert.equal(await fs.readFile(path.join(data,'manifest.json'),'utf8'),'keep existing manifest');assert.equal(await fs.readFile(path.join(data,'raw/asset_010.bin'),'utf8'),'keep raw mod');
 await fs.rm(path.join(data,'menu_music.wav'));const project=await s.editAudio(cue.id,{start:44100,end:88200,gain:50},first.revision);await fs.rm(path.join(data,'menu_music.wav'));
 await s.publishMods();const active=JSON.parse(await run(s.tool('rage-mod-cli'),['--active-build',s.modsRoot,project.source.region,project.source.fingerprint])),manifest=JSON.parse(await run(s.tool('rage-mod-cli'),[path.join(active.directory,'mod.toml')]));
 const info=JSON.parse(await run(s.tool('rage-mod-cli'),['--wav-info',path.join(active.directory,manifest.audio[cue.id])]));assert.equal(info.frames,44100);assert.deepEqual(await fs.readFile(path.join(data,'menu_music.wav')),expected);assert.deepEqual((await fs.readdir(root)).filter(n=>n.startsWith('music-repair-')),[]);
});

test('repair rejects a changed source fingerprint and a non-file cache before publishing',async t=>{
 const {root,data,s}=await fixture(t),archive=path.join(root,'RAGE.BIN');await fs.writeFile(archive,'changed archive');s.state.disc={data,archive,region:'PAL',fingerprint:'sha256:'+'0'.repeat(64)};
 await assert.rejects(repair(s),/changed since extraction/);await assert.rejects(fs.stat(path.join(data,'menu_music.wav')),/ENOENT/);
 await fs.mkdir(path.join(data,'menu_music.wav'));await assert.rejects(repair(s),/regular file/);assert.equal((await fs.stat(path.join(data,'menu_music.wav'))).isDirectory(),true);
});
