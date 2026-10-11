const fs=require('node:fs/promises'),path=require('node:path');
const {ProjectFiles}=require('./project-files.cjs');
const names=['grand-prix','extra-grand-prix','time-attack','custom','load-save','options'];
const labels=['Grand Prix','Extra Grand Prix','Time Attack','Custom','Load / Save','Options'];
function slot(id){const match=/^ui\.title\.([a-z-]+)(\.selected)?$/.exec(id),item=match?names.indexOf(match[1]):-1;if(item<0)throw Error('Unknown title menu image');return {item,selected:Boolean(match[2])};}
async function validate(service,file,signal){const {run}=require('./service.cjs');const png=JSON.parse(await run(service.tool('rage-mod-cli'),['--png',file],{signal}));if(png.width!==112||png.height!==16)throw Error('Title menu images must be 112×16 PNGs');}
async function withImages(service,work){
 service.requireReady();return service.operation('Reading title menu images',signal=>require('./car-view.cjs').withView(service,signal,undefined,async view=>{
  const temp=await fs.mkdtemp(path.join(service.root,'title-images-'));
  try{return await work(view,temp,signal);}finally{await fs.rm(temp,{recursive:true,force:true});}
 }));
}
async function image(service,id,view,temp,signal){
 const {item,selected}=slot(id),file=path.join(temp,item+'-'+Number(selected)+'.png');let origin='built-in';
 const change=service.editor.project.snapshot().changes.find(c=>c.kind==='interface'&&c.id===id&&c.field==='texture');
 const relative=view.manifest.textures?.[id];
 if(change){await fs.writeFile(file,await new ProjectFiles(service.editor.file).read(change.value));origin='project';}
 else if(relative){if(!view.files.includes(relative))throw Error('Missing title menu PNG');await fs.copyFile(path.join(view.root,relative),file);origin='selected packages';}
 else {const {run}=require('./service.cjs');await run(service.tool('rage-mod-cli'),['--title-image',String(item),String(Number(selected)),file],{signal});}
 await validate(service,file,signal);return {file,origin,overridden:Boolean(change)};
}
async function read(service){return withImages(service,async(view,temp,signal)=>{
 const images=[];for(let item=0;item<names.length;item++)for(const selected of [false,true]){
  const id='ui.title.'+names[item]+(selected?'.selected':''),resolved=await image(service,id,view,temp,signal);
  images.push({id,label:labels[item],selected,origin:resolved.origin,overridden:resolved.overridden,png:(await fs.readFile(resolved.file)).toString('base64')});
 }
 return {revision:service.editor.project.revision,images};
});}
async function importImage(service,id,input,revision){slot(id);return withImages(service,async(view,temp,signal)=>{
 const project=service.editor.project;project.checkRevision(revision);
 const stat=await fs.lstat(input);if(!stat.isFile()||stat.isSymbolicLink()||stat.size>32*1024*1024||path.extname(input).toLowerCase()!=='.png')throw Error('Choose a PNG smaller than 32 MiB');
 const frozen=path.join(temp,'import.png');await fs.copyFile(input,frozen);await validate(service,frozen,signal);
 const reference=await new ProjectFiles(service.editor.file).put(frozen,'png');
 return project.edit([{kind:'interface',id,field:'texture',value:reference}],revision);
});}
async function exportImage(service,id,output){slot(id);return withImages(service,async(view,temp,signal)=>{
 const {atomic}=require('./service.cjs'),resolved=await image(service,id,view,temp,signal);await atomic(output,await fs.readFile(resolved.file));return output;
});}
async function reset(service,id,revision){slot(id);return service.editProject([{kind:'interface',id,field:'texture',value:null}],revision);}
module.exports={read,importImage,exportImage,reset,slot};
