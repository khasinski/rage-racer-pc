const fs=require('node:fs/promises'),path=require('node:path');
const {ProjectFiles}=require('./project-files.cjs');
const pattern=/^track\.(big|mid|hi|oval)([1-6])\.(course|terrain)$/;
function slot(id){
 const m=pattern.exec(id);if(!m)throw Error('Invalid track mesh slot');
 return {bank:88+(Number(m[2])-1)*8+['big','mid','hi','oval'].indexOf(m[1])*2,name:m[3]};
}
function checkBank(info,metadata){
 if(info.meshes!==metadata.meshes)throw Error('Track model bank must retain its model count and order');
 const materials=new Set(metadata.materials.map(m=>m.source));materials.add(65535);
 if(info.materials.some(m=>!materials.has(m)))throw Error('Track model uses unknown material slots');
}
async function model(service,id,target,directory,files,signal,expected){
 const {run}=require('./service.cjs');slot(id);
 if(files.includes('mod.toml')){
  const manifest=JSON.parse(await run(service.tool('rage-mod-cli'),[path.join(directory,'mod.toml')],{signal}));
  const relative=manifest.meshes?.[id];
  if(relative){if(!files.includes(relative))throw Error('Missing track model');await fs.copyFile(path.join(directory,relative),target);}
 }
 const change=service.editor?.project.snapshot().changes.find(c=>c.kind==='track'&&c.id===id&&c.field==='mesh');
 if(change)await fs.writeFile(target,await new ProjectFiles(service.editor.file).read(change.value));
 const info=JSON.parse(await run(service.tool('rage-mod-cli'),['--mesh-bank',target],{signal}));
 if(info.meshes!==expected)throw Error('Track model bank must retain its model count and order');
 return info;
}
module.exports={slot,model};
async function validateBuild(service,models,directory,files,signal){
 const {run}=require('./service.cjs'),banks=new Map();
 for(const [id,relative]of Object.entries(models))if(pattern.test(id)){
  const {bank,name}=slot(id);if(!banks.has(bank))banks.set(bank,[]);banks.get(bank).push({name,relative});
 }
 for(const [bank,entries]of banks){
  const folder=path.join(directory,'.track-check-'+bank);await fs.mkdir(folder);
  try{
   const raw=`raw/asset_${String(bank).padStart(3,'0')}.bin`,source=path.join(files.includes(raw)?directory:service.state.disc.data,raw);
   const metadata=JSON.parse(await run(service.tool('rage-racer'),['--config',service.baseConfig,'--set','tools.track_export='+source,'--set','tools.track_bank='+bank,'--set','tools.track_output='+folder],{cwd:service.root,signal,maxOutput:4*1024*1024}));
   for(const {name,relative}of entries){const info=JSON.parse(await run(service.tool('rage-mod-cli'),['--mesh-bank',path.join(directory,relative)],{signal}));checkBank(info,metadata[name]);}
  }finally{await fs.rm(folder,{recursive:true,force:true});}
 }
}
module.exports.validateBuild=validateBuild;
async function withBank(service,id,signal,work){
 const {run}=require('./service.cjs'),{randomUUID}=require('node:crypto'),{bank,name}=slot(id);
 return require('./car-view.cjs').withView(service,signal,undefined,async view=>{
  const directory=path.join(service.root,'track-model-'+randomUUID());await fs.mkdir(directory);
  try{
   const relative=`raw/asset_${String(bank).padStart(3,'0')}.bin`,files=[];
   if(view.files.includes(relative)){await fs.mkdir(path.join(directory,'raw'));await fs.copyFile(path.join(view.root,relative),path.join(directory,relative));files.push(relative);}
   const prefix=id.slice(0,id.lastIndexOf('.'))+'.';
   await require('./track-edits.cjs').apply(service,(service.editor?.project.snapshot().changes||[]).filter(c=>c.kind==='track'&&c.id.startsWith(prefix)),directory,files,signal);
   const source=path.join(files.includes(relative)?directory:service.state.disc.data,relative);
   const metadata=JSON.parse(await run(service.tool('rage-racer'),['--config',service.baseConfig,'--set','tools.track_export='+source,'--set','tools.track_bank='+bank,'--set','tools.track_output='+directory],{cwd:service.root,signal,maxOutput:4*1024*1024}));
   return await work({file:path.join(directory,name+'.rmesh'),directory,metadata:metadata[name],view});
  }finally{await fs.rm(directory,{recursive:true,force:true});}
 });
}
async function importMesh(service,id,input,revision){
 service.requireReady();slot(id);
 return service.operation('Importing track model',signal=>withBank(service,id,signal,async({directory,metadata})=>{
  if(!service.editor)throw Error('Open a source project first');
  const project=service.editor.project;project.checkRevision(revision);
  const stat=await fs.stat(input);if(!stat.isFile()||stat.size>128*1024*1024||path.extname(input).toLowerCase()!=='.rmesh')throw Error('Choose a native RMESH bank smaller than 128 MiB');
  const file=path.join(directory,'import.rmesh');await fs.copyFile(input,file);
  const {run}=require('./service.cjs'),info=JSON.parse(await run(service.tool('rage-mod-cli'),['--mesh-bank',file],{signal}));
  checkBank(info,metadata);
  const reference=await new ProjectFiles(service.editor.file).put(file);
  if(signal.aborted)throw Error('Operation canceled');
  return project.edit([{kind:'track',id,field:'mesh',value:reference}],revision);
 }));
}
async function exportMesh(service,id,output){
 service.requireReady();slot(id);
 return service.operation('Exporting track model',signal=>withBank(service,id,signal,async({file,metadata,view})=>{
  await model(service,id,file,view.root,view.files,signal,metadata.meshes);
  await require('./service.cjs').atomic(output,await fs.readFile(file));return output;
 }));
}
module.exports.importMesh=importMesh;module.exports.exportMesh=exportMesh;

module.exports.checkBank=checkBank;
async function importPart(service,id,part,input,revision){
 service.requireReady();slot(id);if(!Number.isInteger(part)||part<0)throw Error('Invalid track model');
 return service.operation('Importing track model',signal=>withBank(service,id,signal,async({file,directory,metadata,view})=>{
  const project=service.editor?.project;if(!project)throw Error('Open a source project first');project.checkRevision(revision);
  await model(service,id,file,view.root,view.files,signal,metadata.meshes);
  const format=path.extname(input).toLowerCase(),stat=await fs.stat(input);if(!['.obj','.glb','.gltf'].includes(format)||!stat.isFile()||stat.size>128*1024*1024)throw Error('Choose an OBJ, GLB or glTF model smaller than 128 MiB');
  const {run}=require('./service.cjs'),replacement=path.join(directory,'part.rmesh'),output=path.join(directory,'replacement.rmesh');
  let images=[];if(format==='.obj')await run(service.tool('rage-mesh-obj'),['import',input,replacement],{signal});else{const packed=await require('./car-set.cjs').pack(service,input,path.join(directory,'input.glb'),signal);images=JSON.parse(await run(service.tool('rage-mesh-obj'),['import-track-glb',packed,replacement,directory],{signal})).textures;}
  await run(service.tool('rage-mod-cli'),['--replace-mesh',file,String(part),replacement,output],{signal});
  checkBank(JSON.parse(await run(service.tool('rage-mod-cli'),['--mesh-bank',output],{signal})),metadata);
  const reference=await new ProjectFiles(service.editor.file).put(output);
  const textures=format==='.obj'?await importTextures(service,id,part,input,metadata,signal,directory):[];
  for(const image of images){if(!metadata.materials.some(m=>m.source===image))throw Error('GLB image uses an unknown track material');const png=path.join(directory,'texture-'+image+'.png');await run(service.tool('rage-mod-cli'),['--png',png],{signal});textures.push({kind:'track',id:id+'.material.'+image,field:'texture',value:await new ProjectFiles(service.editor.file).put(png,'png')});}
  if(signal.aborted)throw Error('Operation canceled');return project.edit([{kind:'track',id,field:'mesh',value:reference},...textures],revision);
 }));
}
async function exportPart(service,id,part,destination,format='obj'){
 if(!['obj','glb'].includes(format))throw Error('Choose OBJ or GLB export');
 service.requireReady();const {bank,name}=slot(id);if(!Number.isInteger(part)||part<0)throw Error('Invalid track model');
 const course=['Mythical Coast','Over Pass City','Lakeside Gate','The Extreme Oval'][((bank-88)/2)%4],grade=Math.floor((bank-88)/8)+1;
 return service.operation('Exporting track model '+format.toUpperCase(),signal=>withBank(service,id,signal,async({file,directory,metadata,view})=>{
  await model(service,id,file,view.root,view.files,signal,metadata.meshes);
  const {run}=require('./service.cjs'),folder=await require('./export-folder.cjs')(destination,`${course} - Class ${grade} - ${name==='course'?'Course':'Terrain'} model ${part}`,'Track');
  try{if(format==='obj')await run(service.tool('rage-mesh-obj'),['export',file,String(part),path.join(folder,'model.obj')],{signal});
   const geometry=await texturedPart(service,id,part,{file,directory,metadata,view},signal);
   const textures={};for(const [source,texture]of Object.entries(geometry.textures)){const relative='texture-'+source+'.png';await fs.writeFile(path.join(folder,relative),Buffer.from(texture.png,'base64'));textures[source]=relative;}
   if(format==='glb'){await run(service.tool('rage-mesh-obj'),['export-track-glb',file,String(part),path.join(folder,'model.glb'),folder],{signal});for(const relative of Object.values(textures))await fs.rm(path.join(folder,relative));await fs.writeFile(path.join(folder,'README.txt'),'Edit model.glb in Blender and import GLB or glTF on the same track model. Keep material custom properties (rr_material) or rage_N names, normals, vertex colors and UVs. Single static mesh node; no animation or PBR material edits. Embedded PNG textures are imported together with geometry. Up to 64 texture sources, each up to 4096 x 4096; model file up to 128 MiB. Native local Y-up renderer coordinates (4 per game world unit). Geometry changes affect modern rendering only, not the driveable road. Material textures are shared with other models using the same slot.\n');return folder;}
   const mtlFile=path.join(folder,'model.obj.mtl');let mtl=await fs.readFile(mtlFile,'utf8');
   mtl=mtl.replace(/newmtl rage_(\d+)\n/g,(line,word)=>{const texture=textures[Number(word)&65535];return line+(texture?'map_Kd '+texture+'\n':'');});await fs.writeFile(mtlFile,mtl);
   await fs.writeFile(path.join(folder,'track-model.json'),JSON.stringify({format:1,id,part,source:service.editor?.project.snapshot().source,textures},null,2));
await fs.writeFile(path.join(folder,'README.txt'),'Edit model.obj in Blender. Preserve rage_N material names and export triangulated OBJ with normals, UVs and vertex colors. Import it back on the same track model. Coordinates are local Y-up renderer units (4 per game world unit). Edit the included PNG textures too; keep track-model.json beside model.obj to import geometry and textures together. Texture slots are shared by other models using the same material.\n');return folder;}catch(e){await fs.rm(folder,{recursive:true,force:true});throw e;}
 }));
}
module.exports.importPart=importPart;module.exports.exportPart=exportPart;
async function importTextures(service,id,part,input,metadata,signal,staging){
 const {run}=require('./service.cjs'),directory=path.dirname(input),sidecar=path.join(directory,'track-model.json');
 const stat=await fs.lstat(sidecar).catch(e=>{if(e.code==='ENOENT')return null;throw e;});if(!stat)return [];
 if(!stat.isFile()||stat.isSymbolicLink()||stat.size>1024*1024)throw Error('Invalid track model manifest');
 const manifest=JSON.parse(await fs.readFile(sidecar,'utf8')),source=service.editor.project.snapshot().source;
 if(manifest.format!==1||manifest.id!==id||manifest.part!==part||manifest.source?.region!==source.region||manifest.source?.fingerprint!==source.fingerprint||!manifest.textures||typeof manifest.textures!=='object'||Array.isArray(manifest.textures)||Object.keys(manifest.textures).length>4096)throw Error('Track model manifest does not match this source and model');
 const allowed=new Set(metadata.materials.map(m=>String(m.source))),changes=[],store=new ProjectFiles(service.editor.file);
 for(const [slot,relative]of Object.entries(manifest.textures)){
  if(!allowed.has(slot)||relative!=='texture-'+slot+'.png')throw Error('Invalid track texture binding');
  const file=path.join(directory,relative),info=await fs.lstat(file);
  if(!info.isFile()||info.isSymbolicLink()||info.size>128*1024*1024)throw Error('Invalid track texture file');
  const frozen=staging?path.join(staging,'import-texture-'+slot+'.png'):file;
  if(staging)await fs.copyFile(file,frozen);
  await run(service.tool('rage-mod-cli'),['--png',frozen],{signal});
  changes.push({kind:'track',id:id+'.material.'+slot,field:'texture',value:await store.put(frozen,'png')});
 }
 return changes;
}
module.exports.importTextures=importTextures;
async function previewPart(service,id,part){
 service.requireReady();const {name}=slot(id);if(!Number.isInteger(part)||part<0)throw Error('Invalid track model');
 return service.operation('Previewing track model',signal=>withBank(service,id,signal,async({file,directory,metadata,view})=>{
  await model(service,id,file,view.root,view.files,signal,metadata.meshes);
  const geometry=await texturedPart(service,id,part,{file,directory,metadata,view},signal);
  const banks={[name]:{geometry,instances:[{mesh:part,position:[0,0,0],yaw:0}]}};
  const result=require('./track-preview.cjs')(banks);result.previewName=(name==='course'?'Course':'Terrain')+' model '+part;return result;
 }));
}
module.exports.previewPart=previewPart;

async function texturedPart(service,id,part,{file,directory,metadata,view},signal){
 const {bank,name}=slot(id),{run}=require('./service.cjs');
 const geometry=JSON.parse(await run(service.tool('rage-mod-cli'),['--preview-bank',file],{signal,maxOutput:64*1024*1024}));
 if(!geometry.ranges[part]||!geometry.ranges[part][1])throw Error('This track model has no geometry');
 await require('./track-textures.cjs')(service,bank,{[name]:{geometry,instances:[{mesh:part}]}},{[name]:metadata},directory,[],signal,view);
 return geometry;
}

function surfaces(geometry,part){
 const range=geometry.ranges[part];if(!range||!range[1])throw Error('Choose a non-empty track model');
 const used=new Map();for(let i=range[0];i<range[0]+range[1];i++){const source=geometry.vertices[geometry.indices[i]*9+6]&65535;if(source!==65535&&!used.has(source))used.set(source,{source,models:[]});}
 geometry.ranges.forEach(([first,count],model)=>{const seen=new Set();for(let i=first;i<first+count;i++){const source=geometry.vertices[geometry.indices[i]*9+6]&65535;if(used.has(source)&&!seen.has(source)){used.get(source).models.push(model);seen.add(source);}}});
 return [...used.values()];
}
async function withSurfaces(service,id,part,work){
 service.requireReady();slot(id);if(!Number.isInteger(part)||part<0)throw Error('Invalid track model');
 return service.operation('Reading track surfaces',signal=>withBank(service,id,signal,async context=>{
  await model(service,id,context.file,context.view.root,context.view.files,signal,context.metadata.meshes);
  const geometry=await texturedPart(service,id,part,context,signal);
  return work(surfaces(geometry,part),geometry,context,signal);
 }));
}
async function textures(service,id,part){return withSurfaces(service,id,part,(surfaces,geometry,{view})=>{
 const project=service.editor.project.snapshot();return {id,part,revision:project.revision,surfaces:surfaces.filter(s=>geometry.textures[s.source]).map((s,n)=>{
  const key=id+'.material.'+s.source,change=project.changes.find(c=>c.kind==='track'&&c.id===key&&c.field==='texture'),png=geometry.textures[s.source].png,bytes=Buffer.from(png,'base64');
  return {...s,id:key,label:'Surface '+(n+1),png,width:bytes.readUInt32BE(16),height:bytes.readUInt32BE(20),properties:geometry.materials[s.source]?.properties||'inherit auto 1 0 1 1 1 1 0 0 0',materialOverridden:project.changes.some(c=>['car','track'].includes(c.kind)&&c.id===key&&c.field==='material'),overridden:Boolean(change),origin:change?'project':view.manifest.textures?.[key]||view.manifest.textures?.[key+'.variant.0']?'selected packages':'disc'};
 })};
});}
async function importTexture(service,id,part,source,input,revision){return withSurfaces(service,id,part,async(surfaces,geometry,{directory},signal)=>{
 const project=service.editor.project;project.checkRevision(revision);if(!Number.isInteger(source)||!surfaces.some(s=>s.source===source)||!geometry.textures[source])throw Error('Choose a texture used by this model');
 const stat=await fs.lstat(input);if(!stat.isFile()||stat.isSymbolicLink()||stat.size>32*1024*1024||path.extname(input).toLowerCase()!=='.png')throw Error('Choose a PNG smaller than 32 MiB');
 const frozen=path.join(directory,'surface.png');await fs.copyFile(input,frozen);const {run}=require('./service.cjs'),info=JSON.parse(await run(service.tool('rage-mod-cli'),['--png',frozen],{signal}));if(info.width>4096||info.height>4096)throw Error('Track textures must be at most 4096 × 4096 pixels');
 const value=await new ProjectFiles(service.editor.file).put(frozen,'png');if(signal.aborted)throw Error('Operation canceled');return project.edit([{kind:'track',id:id+'.material.'+source,field:'texture',value}],revision);
});}
async function exportTexture(service,id,part,source,output){return withSurfaces(service,id,part,async(surfaces,geometry)=>{
 if(!Number.isInteger(source)||!surfaces.some(s=>s.source===source)||!geometry.textures[source])throw Error('Choose a texture used by this model');await require('./service.cjs').atomic(output,Buffer.from(geometry.textures[source].png,'base64'));return output;
});}
async function resetTexture(service,id,part,source,revision){return withSurfaces(service,id,part,(surfaces,geometry,context,signal)=>{
 if(!Number.isInteger(source)||!surfaces.some(s=>s.source===source)||!geometry.textures[source])throw Error('Choose a texture used by this model');if(signal.aborted)throw Error('Operation canceled');return service.editor.project.edit([{kind:'track',id:id+'.material.'+source,field:'texture',value:null}],revision);
});}
Object.assign(module.exports,{surfaces,textures,importTexture,exportTexture,resetTexture});
