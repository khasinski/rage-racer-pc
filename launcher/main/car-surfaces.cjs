const fs=require('node:fs/promises'),path=require('node:path');
const {ProjectFiles}=require('./project-files.cjs');
async function withSurfaces(service,key,work){
 service.requireReady();if(typeof key!=='string'||!/^car\.(player|rival)\.\d+\.part\.\d+$/.test(key)||key.length>95)throw Error('Invalid car part');
 return service.operation('Reading car surfaces',signal=>require('./car-view.cjs').withView(service,signal,undefined,async view=>{
  const root=await fs.mkdtemp(path.join(service.root,'car-surfaces-'));
  try{
   const folder=await service.exportCarData(key,root,signal,'png',view),metadata=JSON.parse(await fs.readFile(path.join(folder,'materials.json'),'utf8'));
   const [,kind,bank]=key.split('.'),{run}=require('./service.cjs'),table=JSON.parse(await run(service.tool('rage-mod-cli'),['--car-material-usage',kind,path.join(service.state.disc.data,'raw',`asset_${bank.padStart(3,'0')}.bin`),bank],{signal}));
   const stats=JSON.parse(await run(service.tool('rage-mod-cli'),['--mesh',path.join(folder,'model.rmesh')],{signal})),used=new Set(stats.materials.filter(word=>word!==65535).map(word=>word%64));
   const bound=new Map();for(const m of metadata){const slot=table.materials.find(s=>s.page===m.page&&s.clut===m.clut);if(slot&&used.has(m.source)){const file=path.join(folder,'texture-'+m.source+'.png'),exists=await fs.stat(file).catch(e=>{if(e.code==='ENOENT')return null;throw e;});if(!bound.has(slot.id))bound.set(slot.id,{...slot,file:exists?file:null});}}
   return await work([...bound.values()],view,root,signal);
  }finally{await fs.rm(root,{recursive:true,force:true});}
 }));
}
async function read(service,key){return withSurfaces(service,key,async(surfaces,view,root,signal)=>{
 const catalog=JSON.parse(await service.carTool(['--set','tools.car_catalog=true'],signal)),[,family,bank]=key.split('.'),parts=catalog.parts.filter(p=>p.bank===Number(bank)&&p.rival===(family==='rival')),project=service.editor.project.snapshot(),names=JSON.parse(await require('./service.cjs').run(service.tool('rage-save-cli'),['car-names'],{signal})),display=require('./car-names.cjs');
 const list=[];for(const s of surfaces){const changes=project.changes.filter(c=>c.id===s.id||c.id===s.id+'.variant.0'),png=s.file?(await fs.readFile(s.file)).toString('base64'):null,bytes=png?Buffer.from(png,'base64'):null,appearance=changes.find(c=>c.id===s.id+'.variant.0'&&c.field==='material')??changes.find(c=>c.id===s.id&&c.field==='material');list.push({id:s.id,source:s.id,label:'Surface '+(s.slot+1),png,width:bytes?.readUInt32BE(16),height:bytes?.readUInt32BE(20),models:s.models.map(m=>{const part=parts.find(p=>p.part===m);return part?display(part.name,service.state.disc.region,names)+' · part '+m:'Part '+m;}),overridden:changes.some(c=>c.field==='texture'),materialOverridden:Boolean(appearance),inherited:!appearance&&!view.manifest.materials?.[s.id+'.variant.0']&&!view.manifest.materials?.[s.id],materialId:appearance?.id??s.id,properties:appearance?.value??view.manifest.materials?.[s.id+'.variant.0']??view.manifest.materials?.[s.id]??'inherit auto 1 0 1 1 1 1 0 0 0',origin:changes.some(c=>c.field==='texture')?'project':view.manifest.textures?.[s.id+'.variant.0']||view.manifest.textures?.[s.id]?'selected packages':'disc'});}
 return {kind:'car',id:key,part:0,revision:project.revision,surfaces:list};
});}
function surface(surfaces,id){const found=surfaces.find(s=>s.id===id&&s.file);if(!found)throw Error('Choose a texture used by this car part');return found;}
async function importTexture(service,key,id,input,revision){return withSurfaces(service,key,async(surfaces,view,root,signal)=>{
 surface(surfaces,id);const project=service.editor.project;project.checkRevision(revision);const stat=await fs.lstat(input);if(!stat.isFile()||stat.isSymbolicLink()||stat.size>32*1024*1024||path.extname(input).toLowerCase()!=='.png')throw Error('Choose a PNG smaller than 32 MiB');const file=path.join(root,'replacement.png');await fs.copyFile(input,file);const info=JSON.parse(await require('./service.cjs').run(service.tool('rage-mod-cli'),['--png',file],{signal}));if(info.width>4096||info.height>4096)throw Error('Car textures must be at most 4096 × 4096 pixels');const value=await new ProjectFiles(service.editor.file).put(file,'png');if(signal.aborted)throw Error('Operation canceled');const target=project.snapshot().changes.some(c=>c.id===id+'.variant.0'&&c.field==='texture')?id+'.variant.0':id;return project.edit([{kind:id.startsWith('track.')?'track':'car',id:target,field:'texture',value}],revision);
});}
async function exportTexture(service,key,id,output){return withSurfaces(service,key,async surfaces=>{await require('./service.cjs').atomic(output,await fs.readFile(surface(surfaces,id).file));return output;});}
async function resetTexture(service,key,id,revision){return withSurfaces(service,key,(surfaces,view,root,signal)=>{surface(surfaces,id);if(signal.aborted)throw Error('Operation canceled');return service.editor.project.edit(['car','track'].flatMap(kind=>[id,id+'.variant.0'].map(id=>({kind,id,field:'texture',value:null}))),revision);});}
module.exports={read,importTexture,exportTexture,resetTexture};
