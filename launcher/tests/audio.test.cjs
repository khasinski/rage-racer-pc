const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),os=require('node:os'),path=require('node:path');
const {LauncherService,run}=require('../main/service.cjs');
const {ProjectFiles}=require('../main/project-files.cjs');
const tool=path.resolve(__dirname,'../resources/bin/rage-mod-cli'+(process.platform==='win32'?'.exe':''));
function wav({rate=44100,channels=2,bits=16,frames=4410}={}){
 const bytes=frames*channels*bits/8,b=Buffer.alloc(44+bytes+(bytes&1));b.write('RIFF');b.writeUInt32LE(bytes+36+(bytes&1),4);b.write('WAVEfmt ',8);b.writeUInt32LE(16,16);b.writeUInt16LE(1,20);b.writeUInt16LE(channels,22);b.writeUInt32LE(rate,24);b.writeUInt32LE(rate*channels*bits/8,28);b.writeUInt16LE(channels*bits/8,32);b.writeUInt16LE(bits,34);b.write('data',36);b.writeUInt32LE(bytes,40);
 for(let i=0;i<frames;i++)for(let c=0;c<channels;c++){const n=Math.sin(i*2*Math.PI*440/rate);if(bits===8)b[44+i*channels+c]=128+Math.round(n*80);else b.writeInt16LE(Math.round(n*12000),44+(i*channels+c)*2);}return b;
}
async function fixture(t){const root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-audio-test-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));await fs.mkdir(path.join(root,'data'));await fs.writeFile(path.join(root,'RAGE.BIN'),'audio source');await fs.writeFile(path.join(root,'data/menu_music.wav'),wav());const s=new LauncherService({root,bin:path.dirname(tool),config:path.resolve(__dirname,'../../rage-port.ini'),modsRoot:path.join(root,'game-mods')});await s.init();s.requireReady=()=>{};s.state.disc={archive:path.join(root,'RAGE.BIN'),path:path.join(root,'RAGE.BIN'),region:'PAL',data:path.join(root,'data')};await s.project();return {root,s};}
test('compiled WAV import converts mono 22.05 kHz 8-bit to bounded stereo PCM without replacing files',async t=>{
 const {root}=await fixture(t),input=path.join(root,'mono.wav'),output=path.join(root,'stereo.wav');await fs.writeFile(input,wav({rate:22050,channels:1,bits:8,frames:2205}));const before=await fs.readFile(input);
 const info=JSON.parse(await run(tool,['--wav-normalize',input,output]));assert.equal(info.sampleRate,44100);assert.equal(info.channels,2);assert.ok(Math.abs(info.duration-.1)<.001);
 const data=await fs.readFile(output);for(let i=44;i<data.length;i+=4)assert.equal(data.readInt16LE(i),data.readInt16LE(i+2));assert.ok(data.some((v,i)=>i>=44&&v!==0));assert.deepEqual(await fs.readFile(input),before);
 await assert.rejects(run(tool,['--wav-normalize',input,output]),/exit|failure|Cannot/);assert.deepEqual(await fs.readFile(input),before);
 await fs.writeFile(input,Buffer.from('not a wav'));await assert.rejects(run(tool,['--wav-normalize',input,path.join(root,'bad.wav')]),/exit|Cannot/);await assert.rejects(fs.stat(path.join(root,'bad.wav')),/ENOENT/);
 const corrupt=Buffer.from(data);corrupt.writeUInt32LE(0xffffffff,40);await fs.writeFile(input,corrupt);await assert.rejects(run(tool,['--wav-info',input]),/exit|Expected/);await assert.rejects(run(tool,['--wav-normalize',input,path.join(root,'bad.wav')]),/exit|Cannot/);
});
test('real audio service imports immutable WAV, builds semantic override and keeps the previous build on damage',async t=>{
 const {root,s}=await fixture(t),initial=await s.audio(),id=initial.cues[0].id;assert.equal(id,'music.course-select');assert.equal(initial.cues[0].origin,'disc');
 const file=path.join(root,'replacement.wav');await fs.writeFile(file,wav({frames:8820}));let p=await s.importAudio(id,file,initial.revision);await fs.writeFile(file,'source changed');let current=await s.audio();assert.equal(current.cues[0].origin,'project');assert.ok(Math.abs(current.cues[0].duration-.2)<.001);
 await assert.rejects(s.importAudio(id,file,p.revision),/exit|Cannot/);assert.equal((await s.project()).revision,p.revision);
 await assert.rejects(s.resetAudio('missing',p.revision),/Unknown/);await assert.rejects(s.resetAudio(id,initial.revision),/changed/);
 const exported=path.join(root,'export.wav');await s.exportAudio(id,exported);assert.equal((await fs.readFile(exported)).toString('base64'),current.cues[0].wav);
 await s.publishMods();const pointer=await fs.readFile(path.join(s.modsRoot,'active.json')),active=JSON.parse(await run(tool,['--active-build',s.modsRoot,p.source.region,p.source.fingerprint]));const manifest=JSON.parse(await run(tool,[path.join(active.directory,'mod.toml')]));assert.match(manifest.audio[id],/^audio\/project\/.+\.wav$/);assert.equal((await fs.readFile(path.join(active.directory,manifest.audio[id]))).toString('base64'),current.cues[0].wav);
 const stored=new ProjectFiles(s.editor.file),change=p.changes.find(c=>c.kind==='audio');await fs.chmod(path.join(stored.root,change.value.slice(7)),0o644);await fs.writeFile(path.join(stored.root,change.value.slice(7)),'damaged');await assert.rejects(s.publishMods(),/damaged/);assert.deepEqual(await fs.readFile(path.join(s.modsRoot,'active.json')),pointer);
 p=await s.resetAudio(id,(await s.project()).revision);assert.equal((await s.audio()).cues[0].origin,'disc');
});
test('Audio workspace uses named playable WAV and explains import, scope and preview volume',async()=>{
 const {audioWorkspace}=await import('../renderer/audio.mjs'),html=audioWorkspace({revision:4,cues:[{id:'music.course-select',label:'Course / car select music',wav:'AAAA',duration:12,origin:'project',overridden:true}]});
 for(const token of ['<audio controls','Import WAV','Export WAV','data-reset-audio','Preview volume','44.1 kHz','not editable here yet','data-revision="4"'])assert.ok(html.includes(token),token);
});
test('audio packages resolve semantic conflicts and project reset restores the selected provider',async t=>{
 const {root,s}=await fixture(t),id='music.course-select',mods=[];
 for(const [index,frames]of [4410,8820].entries()){
  const folder=path.join(root,'package-'+index);await fs.mkdir(path.join(folder,'audio'),{recursive:true});await fs.writeFile(path.join(folder,'audio/song.wav'),wav({frames}));await fs.writeFile(path.join(folder,'mod.toml'),`[mod]\nid="music-${index}"\n[audio]\n"${id}"="audio/song.wav"\n`);
  await s.importMod(folder);const mod=s.state.mods.at(-1);mods.push(mod.id);await s.toggleMod(mod.id,true);
 }
 assert.equal(s.conflicts()[0].key,'audio:'+id);await assert.rejects(s.audio(),/priority|conflict|provider/i);
 await s.resolveConflict('audio:'+id,mods[1]);assert.equal((await s.audio()).cues[0].origin,'selected packages');assert.ok(Math.abs((await s.audio()).cues[0].duration-.2)<.001);
 const file=path.join(root,'project.wav');await fs.writeFile(file,wav({frames:13230}));let p=await s.importAudio(id,file,(await s.project()).revision);assert.ok(Math.abs((await s.audio()).cues[0].duration-.3)<.001);
 p=await s.resetAudio(id,p.revision);assert.ok(Math.abs((await s.audio()).cues[0].duration-.2)<.001);
 p=await s.projectHistory('undo',p.revision);assert.ok(Math.abs((await s.audio()).cues[0].duration-.3)<.001);
 p=await s.projectHistory('redo',p.revision);await s.publishMods();
 const active=JSON.parse(await run(tool,['--active-build',s.modsRoot,p.source.region,p.source.fingerprint]));const manifest=JSON.parse(await run(tool,[path.join(active.directory,'mod.toml')]));assert.match(manifest.audio[id],/^audio\/provider-/);assert.equal(JSON.parse(await run(tool,['--wav-info',path.join(active.directory,manifest.audio[id])])).duration,.2);
});
