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
async function effective(service,view,temp,signal){
 const change=service.editor.project.snapshot().changes.find(c=>c.kind==='audio'&&c.id===ID&&c.field==='file');
 let file=path.join(service.state.disc.data,'menu_music.wav'),origin='disc';
 if(change){file=path.join(temp,'effective.wav');await fs.writeFile(file,await new ProjectFiles(service.editor.file).read(change.value));origin='project';}
 else if(view.manifest.audio?.[ID]){const relative=view.manifest.audio[ID];if(!view.files.includes(relative))throw Error('Missing menu music WAV');file=path.join(view.root,relative);origin='selected packages';}
 const {run}=require('./service.cjs');const info=JSON.parse(await run(service.tool('rage-mod-cli'),['--wav-info',file],{signal}));
 return {file,origin,overridden:Boolean(change),...info};
}
async function read(service){return withAudio(service,async(view,temp,signal)=>{const audio=await effective(service,view,temp,signal);return {revision:service.editor.project.revision,cues:[{id:ID,label:'Course / car select music',...audio,file:undefined,wav:(await fs.readFile(audio.file)).toString('base64')}]};});}
async function importAudio(service,id,input,revision){
 slot(id);return withAudio(service,async(view,temp,signal)=>{
  const p=service.editor.project;p.checkRevision(revision);
  const stat=await fs.lstat(input);if(!stat.isFile()||stat.isSymbolicLink()||stat.size>128*1024*1024||path.extname(input).toLowerCase()!=='.wav')throw Error('Choose a WAV file under 128 MiB');
  const copy=path.join(temp,'input.wav'),output=path.join(temp,'normalized.wav');await fs.copyFile(input,copy);
  const {run}=require('./service.cjs');await run(service.tool('rage-mod-cli'),['--wav-normalize',copy,output],{signal});
  const ref=await new ProjectFiles(service.editor.file).put(output,'wav');if(signal.aborted)throw Error('Operation canceled');
  return p.edit([{kind:'audio',id,field:'file',value:ref}],revision);
 });
}
async function exportAudio(service,id,output){slot(id);return withAudio(service,async(view,temp,signal)=>{
 const audio=await effective(service,view,temp,signal),{atomic}=require('./service.cjs');await atomic(output,await fs.readFile(audio.file));return true;
});}
async function reset(service,id,revision){slot(id);return withAudio(service,()=>service.editor.project.edit([{kind:'audio',id,field:'file',value:null}],revision));}
module.exports={read,importAudio,exportAudio,reset,slot};
