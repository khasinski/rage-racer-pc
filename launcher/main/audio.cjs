const fs=require('node:fs/promises'),path=require('node:path');
const {ProjectFiles}=require('./project-files.cjs');
const ID='music.course-select';
function slot(id){if(id!==ID)throw Error('Unknown editable audio cue');return id;}
async function withAudio(service,work){
 service.requireReady();return service.operation('Reading menu music',signal=>require('./car-view.cjs').withView(service,signal,undefined,async view=>{
  const temp=await fs.mkdtemp(path.join(service.root,'audio-'));
  try{return await work(view,temp,signal);}finally{await fs.rm(temp,{recursive:true,force:true});}
 }));
}
async function original(service,view,temp,signal){
 const change=service.editor.project.snapshot().changes.find(c=>c.kind==='audio'&&c.id===ID&&c.field==='file');
 let file=path.join(service.state.disc.data,'menu_music.wav'),origin='disc';
 if(change){if(typeof change.value!=='string'||!change.value.endsWith('.wav'))throw Error('Invalid audio reference');file=path.join(temp,'effective.wav');await fs.writeFile(file,await new ProjectFiles(service.editor.file).read(change.value));origin='project';}
 else if(view.manifest.audio?.[ID]){const relative=view.manifest.audio[ID];if(!view.files.includes(relative))throw Error('Missing menu music WAV');file=path.join(view.root,relative);origin='selected packages';}
 const {run}=require('./service.cjs');const info=JSON.parse(await run(service.tool('rage-mod-cli'),['--wav-info',file],{signal}));
 return {file,origin,overridden:Boolean(change),...info};
}
function parameters(changes,frames){
 const fields=new Map(changes.filter(c=>c.kind==='audio'&&c.id===ID).map(c=>[c.field,c.value]));
 for(const key of fields.keys())if(!['file','start','end','gain'].includes(key))throw Error('Unknown audio property');
 const start=fields.get('start')??0,end=fields.get('end')??frames,gain=fields.get('gain')??100;
 if(!Number.isSafeInteger(start)||!Number.isSafeInteger(end)||start<0||start>=end||end>frames||!Number.isSafeInteger(gain)||gain<0||gain>200)throw Error('Choose a valid audio range and volume from 0 to 200%');
 return {start,end,gain};
}
async function effective(service,view,temp,signal){
 const audio=await original(service,view,temp,signal),changes=service.editor.project.snapshot().changes,params=parameters(changes,audio.frames);
 const sourceFrames=audio.frames;let file=audio.file,clippedSamples=0;
 if(params.start!==0||params.end!==sourceFrames||params.gain!==100){
  file=path.join(temp,'edited.wav');const {run}=require('./service.cjs');const info=JSON.parse(await run(service.tool('rage-mod-cli'),['--wav-edit',audio.file,file,String(params.start),String(params.end),String(params.gain)],{signal}));clippedSamples=info.clippedSamples;
 }
 const overridden=changes.some(c=>c.kind==='audio'&&c.id===ID);
 return {...audio,file,origin:overridden?'project':audio.origin,overridden,sourceFrames,...params,frames:params.end-params.start,duration:(params.end-params.start)/44100,clippedSamples};
}
async function read(service){return withAudio(service,async(view,temp,signal)=>{const audio=await effective(service,view,temp,signal);return {revision:service.editor.project.revision,cues:[{id:ID,label:'Course / car select music',...audio,file:undefined,wav:(await fs.readFile(audio.file)).toString('base64'),waveform:JSON.parse(await require('./service.cjs').run(service.tool('rage-mod-cli'),['--wav-waveform',audio.file],{signal}))}]};});}
async function importAudio(service,id,input,revision){
 slot(id);return withAudio(service,async(view,temp,signal)=>{
  const p=service.editor.project;p.checkRevision(revision);
  const stat=await fs.lstat(input);if(!stat.isFile()||stat.isSymbolicLink()||stat.size>128*1024*1024||path.extname(input).toLowerCase()!=='.wav')throw Error('Choose a WAV file under 128 MiB');
  const copy=path.join(temp,'input.wav'),output=path.join(temp,'normalized.wav');await fs.copyFile(input,copy);
  const {run}=require('./service.cjs');await run(service.tool('rage-mod-cli'),['--wav-normalize',copy,output],{signal});
  const ref=await new ProjectFiles(service.editor.file).put(output,'wav');if(signal.aborted)throw Error('Operation canceled');
  return p.edit([{kind:'audio',id,field:'file',value:ref},...['start','end','gain'].map(field=>({kind:'audio',id,field,value:null}))],revision);
 });
}
async function exportAudio(service,id,output){slot(id);return withAudio(service,async(view,temp,signal)=>{
 const audio=await effective(service,view,temp,signal),{atomic}=require('./service.cjs');await atomic(output,await fs.readFile(audio.file));return true;
});}
async function reset(service,id,revision){slot(id);return withAudio(service,()=>service.editor.project.edit(['file','start','end','gain'].map(field=>({kind:'audio',id,field,value:null})),revision));}
async function edit(service,id,values,revision){
 slot(id);if(!values||Object.keys(values).length!==3||!['start','end','gain'].every(k=>Object.hasOwn(values,k)))throw Error('Invalid audio settings');
 return withAudio(service,async(view,temp,signal)=>{const p=service.editor.project;p.checkRevision(revision);const audio=await original(service,view,temp,signal);
  parameters(Object.entries(values).map(([field,value])=>({kind:'audio',id,field,value})),audio.frames);
  return p.edit(Object.entries(values).map(([field,value])=>({kind:'audio',id,field,value:value===({start:0,end:audio.frames,gain:100})[field]?null:value})),revision);
 });
}
async function apply(service,changes,staging,files){
 const edits=changes.filter(c=>c.kind==='audio');if(!edits.length)return {};
 if(edits.some(c=>c.id!==ID))throw Error('Unknown editable audio cue');
 const {run}=require('./service.cjs');let manifest={};
 try{await fs.access(path.join(staging,'mod.toml'));manifest=JSON.parse(await run(service.tool('rage-mod-cli'),[path.join(staging,'mod.toml')]));}catch(e){if(e.code!=='ENOENT')throw e;}
 const temp=await fs.mkdtemp(path.join(staging,'.audio-'));
 try{
  const change=edits.find(c=>c.field==='file');let source=path.join(service.state.disc.data,'menu_music.wav');
  if(change){if(typeof change.value!=='string'||!change.value.endsWith('.wav'))throw Error('Invalid audio reference');source=path.join(temp,'source.wav');await fs.writeFile(source,await new ProjectFiles(service.editor.file).read(change.value));}
  else if(manifest.audio?.[ID])source=path.join(staging,manifest.audio[ID]);
  const info=JSON.parse(await run(service.tool('rage-mod-cli'),['--wav-info',source])),params=parameters(edits,info.frames);
  const relative='audio/project/menu.wav',output=path.join(staging,relative);await fs.mkdir(path.dirname(output),{recursive:true});
  await run(service.tool('rage-mod-cli'),['--wav-edit',source,output,String(params.start),String(params.end),String(params.gain)]);
  if(!files.includes(relative))files.push(relative);return {[ID]:relative};
 }finally{await fs.rm(temp,{recursive:true,force:true});}
}
module.exports={read,importAudio,exportAudio,reset,slot,edit,apply};
