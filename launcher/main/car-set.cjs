const fs=require('node:fs/promises'),path=require('node:path'),{randomUUID}=require('node:crypto');
async function car(service,key,signal){
 if(typeof key!=='string'||!/^car\.(player|rival)\.\d+\.part\.\d+$/.test(key))throw Error('Invalid car');
 const catalog=JSON.parse(await service.carTool(['--set','tools.car_catalog=true'],signal));
 const body=catalog.parts.find(p=>p.key===key&&(p.rival?p.part%5===0:p.part===0));if(!body)throw Error('Choose a complete car');return body;
}
async function table(service,c,signal){const {run}=require('./service.cjs');return JSON.parse(await run(service.tool('rage-mod-cli'),['--car-materials',c.rival?'rival':'player',path.join(service.state.disc.data,'raw',`asset_${String(c.bank).padStart(3,'0')}.bin`),String(c.bank)],{signal})).materials;}
async function boundedFile(root,relative,max){
 if(typeof relative!=='string'||!relative||relative.includes('\\')||relative.split('/').some(p=>!p||p==='.'||p==='..')||path.isAbsolute(relative))throw Error('Invalid car set file path');
 let file=root;for(const part of relative.split('/')){file=path.join(file,part);if((await fs.lstat(file)).isSymbolicLink())throw Error('Car sets cannot contain symbolic links');}
 const stat=await fs.stat(file);if(!stat.isFile()||stat.size>max)throw Error('Car set file is too large or not a file');return file;
}
exports.pack=async(service,file,output,signal)=>{const stat=await fs.stat(file);if(!stat.isFile()||stat.size>128*1024*1024)throw Error('Choose a model file up to 128 MiB');if(['.glb','.gltf'].includes(path.extname(file).toLowerCase()))await require('./service.cjs').run(service.tool('rage-mesh-obj'),['pack-gltf',file,output],{signal});else await fs.copyFile(file,output);return output;};
exports.exportSet=async(service,key,destination)=>{
 service.requireReady();return service.operation('Exporting complete car',signal=>require('./car-view.cjs').withView(service,signal,undefined,async view=>{
  const c=await car(service,key,signal),materials=await table(service,c,signal),target=path.join(destination,'car-set-'+randomUUID());await fs.mkdir(target);
  try{
   const manifest={format:1,region:service.state.disc.region,key,parts:{},textures:{}};await fs.mkdir(path.join(target,'textures'));
   for(const [role,offset]of [['body',0],['front-wheels',2],['rear-wheels',3]]){
    const partKey=`car.${c.rival?'rival':'player'}.${c.bank}.part.${c.part+offset}`;
    const folder=await service.exportCarData(partKey,target,signal,'png',view),named=path.join(target,role);await fs.rename(folder,named);
    manifest.parts[role]=role+'/model.obj';
    const meta=JSON.parse(await fs.readFile(path.join(named,'materials.json')));let mtl=await fs.readFile(path.join(named,'model.obj.mtl'),'utf8');
    for(const m of meta){
     const material=materials.find(t=>t.page===m.page&&t.clut===m.clut);const local='texture-'+m.source+'.png';
     if(!material||!mtl.includes('map_Kd '+local+'\n'))continue;
     const relative='textures/material-'+material.slot+'.png';
     if(!manifest.textures[material.slot]){await fs.copyFile(path.join(named,local),path.join(target,relative));manifest.textures[material.slot]=relative;}
     mtl=mtl.replaceAll('map_Kd '+local+'\n','map_Kd ../'+relative+'\n');
    }
    await fs.writeFile(path.join(named,'model.obj.mtl'),mtl);
    for(const name of await fs.readdir(named))if(!['model.obj','model.obj.mtl'].includes(name))await fs.rm(path.join(named,name));
   }
   await fs.writeFile(path.join(target,'car-set.json'),JSON.stringify(manifest,null,2));
   const {run}=require('./service.cjs'),raw=path.join(service.state.disc.data,'raw',`asset_${String(c.bank).padStart(3,'0')}.bin`);
   const layout=JSON.parse(await run(service.tool('rage-mod-cli'),c.rival?['--rival-layout',raw,String(c.part)]:['--car-layout',raw],{signal}));
   const parts=[],folders=[];
   try{
    for(const offset of [0,2,3]){
     const id=`car.${c.rival?'rival':'player'}.${c.bank}.part.${c.part+offset}`;
     const folder=await service.exportCarData(id,target,signal,'png',view,'glb');folders.push(folder);parts.push({id,file:path.join(folder,'model.glb')});
    }
    const instances=layout.instances.map((instance,i)=>{
     const mesh=[0,2,3].indexOf(instance.part-c.part);if(mesh<0)throw Error('Unknown car layout part');
     return {mesh,name:mesh===0?'body':mesh===2?'rear-wheels':`front-wheel-${i}`,position:instance.position,scale:instance.scale};
    });
    const project=await service.loadProject();
    await run(service.tool('rage-mesh-obj'),['assemble-glb',path.join(target,'car.glb')],{signal,input:JSON.stringify({source:{region:manifest.region,key,fingerprint:project?.document.source.fingerprint||null},parts,instances})});
   }finally{for(const folder of folders)await fs.rm(folder,{recursive:true,force:true});}
   await fs.writeFile(path.join(target,'README.txt'),'car.glb contains the assembled car with embedded PNG textures and separate body/wheel nodes for editing in Blender. Native game units, Y-up. Keep custom properties and shared front-wheel mesh; import the GLB on the same car slot.\nEdit the three OBJ files and the shared PNG textures, then Import car set on the same slot.\nKeep rage_* material names, UVs and normals. Export triangulated OBJ.\nShared PNG files are referenced by body and wheel materials; edit each shared image once.\ncar-set.json maps the files to this disc version and car slot. Other slots require material remapping.\n');
   return target;
  }catch(e){await fs.rm(target,{recursive:true,force:true});throw e;}
 }));
};
exports.importSet=async(service,key,source,revision,owner)=>{
 service.requireReady();return service.operation('Importing complete car',async signal=>{
  if(signal.aborted||owner?.cancelled)throw Error('Operation canceled');
  const {run}=require('./service.cjs'),c=await car(service,key,signal);
  if(revision!==undefined)(await service.loadProject()).checkRevision(revision);
  const materials=await table(service,c,signal),staging=path.join(service.root,'car-set-import-'+randomUUID());await fs.mkdir(path.join(staging,'meshes'),{recursive:true});await fs.mkdir(path.join(staging,'textures'));
  try{
   const whole=(await fs.stat(source)).isFile();let manifest;
   if(whole){
    if(!['.glb','.gltf'].includes(path.extname(source).toLowerCase()))throw Error('Choose a complete GLB or glTF car');
    source=await exports.pack(service,source,path.join(staging,'source.glb'),signal);
    const raw=path.join(service.state.disc.data,'raw',`asset_${String(c.bank).padStart(3,'0')}.bin`);
    const layout=JSON.parse(await run(service.tool('rage-mod-cli'),c.rival?['--rival-layout',raw,String(c.part)]:['--car-layout',raw],{signal}));
    const parts=[0,2,3].map(offset=>({id:`car.${c.rival?'rival':'player'}.${c.bank}.part.${c.part+offset}`}));
    const instances=layout.instances.map(i=>({mesh:[0,2,3].indexOf(i.part-c.part),position:i.position,scale:i.scale}));
    const project=await service.loadProject();
    await run(service.tool('rage-mesh-obj'),['separate-glb',source,staging],{signal,input:JSON.stringify({source:{region:service.state.disc.region,key,fingerprint:project?.document.source.fingerprint||null},parts,instances})});
    source=staging;manifest={format:1,region:service.state.disc.region,key,parts:{body:'part-0.glb','front-wheels':'part-1.glb','rear-wheels':'part-2.glb'},textures:{}};
   }else manifest=JSON.parse(await fs.readFile(await boundedFile(source,'car-set.json',65536),'utf8'));
   if(manifest.format!==1||manifest.region!==service.state.disc.region||manifest.key!==key)throw Error('Choose a car set exported for this slot and game version. Cross-slot material remapping is not supported yet.');
   if(!manifest.parts||!manifest.textures||Array.isArray(manifest.textures)||Object.keys(manifest.textures).length>64)throw Error('Invalid car set manifest');
   const meshes=[],textures=[],embedded={};
   for(const [role,offset]of [['body',0],['front-wheels',2],['rear-wheels',3]]){
    const file=await boundedFile(source,manifest.parts[role],128*1024*1024),ext=path.extname(file).toLowerCase(),relative='meshes/'+role+'.rmesh',out=path.join(staging,relative);
    let images=[];
    const imageDir=path.join(staging,'images-'+role);
    if(ext==='.glb'||ext==='.gltf'){await fs.mkdir(imageDir);const packed=await exports.pack(service,file,path.join(imageDir,'source.glb'),signal);images=JSON.parse(await run(service.tool('rage-mesh-obj'),['import-glb',packed,out,imageDir],{signal})).textures;}else if(ext==='.obj')await run(service.tool('rage-mesh-obj'),['import',file,out],{signal});else if(ext==='.rmesh')await fs.copyFile(file,out);else throw Error('Car set models must be OBJ, GLB, glTF or RRMESH');
    const partKey=`car.${c.rival?'rival':'player'}.${c.bank}.part.${c.part+offset}`;
    await require('./car-material-bindings.cjs')(service,partKey,out,staging,role,signal);
    meshes.push(`"${partKey}" = "${relative}"`);
    if(images.length){
     const binding=path.join(staging,'bindings-'+role+'.rmesh'),meta=JSON.parse(await service.carTool(['--set','tools.car_export='+partKey,'--set','tools.car_output='+binding],signal));await fs.rm(binding,{force:true});
     for(const image of images){
      const original=meta.materials.find(m=>m.source===image),material=original&&materials.find(m=>m.page===original.page&&m.clut===original.clut);if(!material)throw Error('GLB image has no matching car texture source');
      const png=path.join(imageDir,`texture-${image}.png`),previous=embedded[material.slot]||manifest.textures[material.slot];
      if(previous){const existing=embedded[material.slot]||await boundedFile(source,previous,32*1024*1024);if(!(await fs.readFile(existing)).equals(await fs.readFile(png)))throw Error('Different GLB images map to the same car surface');}
      else embedded[material.slot]=png;
     }
    }
   }
   for(const [slot,relative]of Object.entries({...manifest.textures,...embedded})){
    const material=materials.find(m=>String(m.slot)===slot);if(!material)throw Error('Unknown car texture slot');
    const file=embedded[slot]||await boundedFile(source,relative,32*1024*1024),bytes=await fs.readFile(file);
    if(bytes.length<24||!bytes.subarray(0,8).equals(Buffer.from([137,80,78,71,13,10,26,10]))||!bytes.readUInt32BE(16)||!bytes.readUInt32BE(20)||bytes.readUInt32BE(16)>4096||bytes.readUInt32BE(20)>4096)throw Error('Car textures must be PNG images up to 4096 × 4096');
    const output='textures/material-'+slot+'.png',staged=path.join(staging,output);await fs.copyFile(file,staged);
    try{await run(service.tool('rage-mod-cli'),['--png',staged],{signal});}catch(e){if(signal.aborted)throw e;throw Error(`Cannot import texture ${relative}: the game could not decode this PNG.`);}
    textures.push(`"${material.id}" = "${output}"`);
   }
   await fs.writeFile(path.join(staging,'mod.toml'),'[meshes]\n'+meshes.join('\n')+'\n[textures]\n'+textures.join('\n')+'\n');
   await service.loadProject();
   if(service.editor){
    const project=service.editor.project,revision=project.revision;
    const parsed=JSON.parse(await run(service.tool('rage-mod-cli'),[path.join(staging,'mod.toml')],{signal}));
    const store=new (require('./project-files.cjs').ProjectFiles)(service.editor.file),changes=[];
    for(const [field,entries,type]of [['mesh',parsed.meshes,'rmesh'],['texture',parsed.textures,'png']]){
     for(const [id,relative]of Object.entries(entries))changes.push({kind:'car',id,field,value:await store.put(path.join(staging,relative),type)});
    }
    if(signal.aborted)throw Error('Operation canceled');return project.edit(changes,revision);
   }
   const names=JSON.parse(await run(service.tool('rage-save-cli'),['car-names'],{signal})),name=require('./car-names.cjs')(c.name,service.state.disc.region,names);
   return (await service.installModFiles(staging,signal,name+' · complete car')).id;
  }finally{await fs.rm(staging,{recursive:true,force:true});}
 },true,owner);
};

exports.prepare=async(service,key,file)=>service.operation('Inspecting car import',async signal=>{
 const {run}=require('./service.cjs'),c=await car(service,key,signal),project=await service.loadProject();if(!project)throw Error('Import a game source before mapping a car');
 if(service.carImport)await fs.rm(service.carImport.folder,{recursive:true,force:true});service.carImport=null;
 const folder=path.join(service.root,'car-map-'+randomUUID());await fs.mkdir(folder);
 try{
  const candidate=path.join(folder,'source.glb'),stat=await fs.stat(file);if(!stat.isFile()||stat.size>128*1024*1024)throw Error('Model exceeds the 128 MiB limit');await exports.pack(service,file,candidate,signal);
  const inspection=JSON.parse(await run(service.tool('rage-mesh-obj'),['inspect-glb',candidate],{signal}));
  const raw=path.join(service.state.disc.data,'raw',`asset_${String(c.bank).padStart(3,'0')}.bin`),layout=JSON.parse(await run(service.tool('rage-mod-cli'),c.rival?['--rival-layout',raw,String(c.part)]:['--car-layout',raw],{signal}));
  const roles=layout.instances.map((instance,index)=>({instance:index,part:`car.${c.rival?'rival':'player'}.${c.bank}.part.${instance.part}`,label:instance.part===c.part?'Body':instance.part===c.part+3?'Rear wheels':instance.position[0]<0?'Front left wheel':'Front right wheel'}));
  if(inspection.nodes.length!==roles.length)throw Error(`This car needs ${roles.length} mesh instances; the file has ${inspection.nodes.length}. Keep both front wheels linked to one mesh.`);
  const available=new Map();
  for(const [offset,label]of [[0,'body'],[2,'front wheels'],[3,'rear wheels']]){
   const original=path.join(folder,'original.rmesh');await service.carTool(['--set',`tools.car_export=car.${c.rival?'rival':'player'}.${c.bank}.part.${c.part+offset}`,'--set','tools.car_output='+original],signal);
   const glb=path.join(folder,'original.glb');await run(service.tool('rage-mesh-obj'),['export-glb',original,'0',glb],{signal});
   const stats=JSON.parse(await run(service.tool('rage-mesh-obj'),['inspect-glb',glb],{signal}));for(const {word}of stats.materials){if(!available.has(word))available.set(word,new Set());available.get(word).add(label);}await fs.rm(original,{force:true});await fs.rm(glb,{force:true});
  }
  const materials=[...available].map(([word,parts],i)=>({word,label:`Material ${i+1} · ${(word&65535)===65535?'inner surface':(word&65535)>>>6===1?'glass':'texture '+((word&65535)%64+1)} · ${[...parts].join(', ')}`,part:`car.${c.rival?'rival':'player'}.${c.bank}.part.${c.part+({body:0,'front wheels':2,'rear wheels':3})[[...parts][0]]}`}));
  const token=randomUUID();service.carImport={token,folder,key,revision:project.revision,source:{...project.document.source,key},roles,materials,inspection};
  return {token,key,name:c.name,region:service.state.disc.region,revision:project.revision,nodes:inspection.nodes,materials:inspection.materials,roles,choices:materials};
 }catch(e){await fs.rm(folder,{recursive:true,force:true});throw e;}
});
exports.complete=async(service,token,mapping)=>{
 const candidate=service.carImport;if(!candidate||candidate.cancelled||candidate.token!==token)throw Error('Car import expired; select the file again');
 const file=await service.operation('Mapping car parts',async signal=>{
  if(signal.aborted||candidate.cancelled)throw Error('Operation canceled');
  const project=await service.loadProject();if(!project||project.revision!==candidate.revision||project.document.source.fingerprint!==candidate.source.fingerprint)throw Error('Project changed; select the car file again');
  if(!mapping||!Array.isArray(mapping.nodes)||mapping.nodes.length!==candidate.inspection.nodes.length||!Array.isArray(mapping.materials)||mapping.materials.length!==candidate.inspection.materials.length)throw Error('Assign every part and material');
  const instances=new Set(),nodes=new Set(),materials=new Set();
  for(const node of mapping.nodes){if(!Number.isInteger(node.node)||!candidate.inspection.nodes.some(n=>n.node===node.node)||nodes.has(node.node)||!Number.isInteger(node.instance)||!candidate.roles.some(r=>r.instance===node.instance)||instances.has(node.instance))throw Error('Each part needs a different role');nodes.add(node.node);instances.add(node.instance);}
  for(const material of mapping.materials){if(!Number.isInteger(material.material)||!candidate.inspection.materials.some(m=>m.material===material.material)||materials.has(material.material)||!candidate.materials.some(m=>m.word===material.word))throw Error('Choose original car materials');materials.add(material.material);}
  const output=path.join(candidate.folder,'mapped-'+randomUUID()+'.glb');await require('./service.cjs').run(service.tool('rage-mesh-obj'),['map-glb',path.join(candidate.folder,'source.glb'),output],{signal,input:JSON.stringify({source:candidate.source,nodes:mapping.nodes.map(n=>({...n,part:candidate.roles.find(r=>r.instance===n.instance).part})),materials:mapping.materials})});return output;
 },true,candidate);
 try{if(candidate.cancelled)throw Error('Operation canceled');const result=await exports.importSet(service,candidate.key,file,candidate.revision,candidate);if(service.carImport===candidate)service.carImport=null;await fs.rm(candidate.folder,{recursive:true,force:true});return result;}finally{await fs.rm(file,{force:true});}
};
exports.cancel=async(service,token)=>{
 const candidate=service.carImport;if(!candidate||candidate.token!==token)return true;candidate.cancelled=true;
 const job=service.busy;if(job?.owner===candidate){job.controller.abort();await job.done;}
 if(service.carImport===candidate)service.carImport=null;await fs.rm(candidate.folder,{recursive:true,force:true});return true;
};
exports.preview=async(service,token,node)=>{const candidate=service.carImport;return service.operation('Previewing imported car part',async signal=>{
 if(signal.aborted||candidate?.cancelled)throw Error('Operation canceled');
 if(!candidate||candidate.cancelled||candidate.token!==token||!Number.isInteger(node)||!candidate.inspection.nodes.some(n=>n.node===node))throw Error('Choose a part in the current car import');
 if(candidate.inspection.materials.length>64)throw Error('Part preview supports at most 64 materials; reduce unused material variants in the 3D editor');
 candidate.previews??=new Map();if(candidate.previews.has(node))return candidate.previews.get(node);
 const {run}=require('./service.cjs'),folder=path.join(candidate.folder,'preview-'+node);await fs.mkdir(folder);
 try{
  const glb=path.join(folder,'part.glb'),mesh=path.join(folder,'part.rmesh');
  await run(service.tool('rage-mesh-obj'),['map-glb',path.join(candidate.folder,'source.glb'),glb],{signal,input:JSON.stringify({source:candidate.source,nodes:[],materials:candidate.inspection.materials.map((m,i)=>({material:m.material,word:i})),preview_node:node})});
  const info=JSON.parse(await run(service.tool('rage-mesh-obj'),['import-glb',glb,mesh,folder],{signal})),geometry=JSON.parse(await run(service.tool('rage-mod-cli'),['--preview',mesh],{signal,maxOutput:16*1024*1024}));geometry.textures={};
  for(const source of info.textures){const png=path.join(folder,'texture-'+source+'.png');await run(service.tool('rage-mod-cli'),['--png',png],{signal});geometry.textures[source]={png:(await fs.readFile(png)).toString('base64')};}
  geometry.previewName=candidate.inspection.nodes.find(n=>n.node===node).name;geometry.previewNote='Imported geometry, vertex colours and embedded textures. Material assignments have not been applied yet. Lighting is simplified for inspection.';candidate.previews.set(node,geometry);return geometry;
 }finally{await fs.rm(folder,{recursive:true,force:true});}
},true,candidate);};
exports.texture=async(service,token,word)=>{const candidate=service.carImport;return service.operation('Previewing target material',async signal=>{
 if(signal.aborted||candidate?.cancelled)throw Error('Operation canceled');
 const material=candidate?.materials.find(m=>m.word===word);if(!candidate||candidate.cancelled||candidate.token!==token||!material)throw Error('Choose a material in the current car import');
 (await service.loadProject()).checkRevision(candidate.revision);candidate.textures??=new Map();const id=word&65535,key=material.part+':'+id;if(candidate.textures.has(key))return {...candidate.textures.get(key),label:material.label};
 const result={label:material.label,note:'Target car texture, including current project and selected mod replacements.'};if(id===65535||id>>>6===1){result.color=id===65535?'#28292e':'#314459';candidate.textures.set(key,result);return result;}
 const folder=path.join(candidate.folder,'material-'+word);await fs.mkdir(folder);
 try{return await require('./car-view.cjs').withView(service,signal,undefined,async view=>{
  const meta=JSON.parse(await service.carTool(['--set','tools.car_export='+material.part,'--set','tools.car_output='+path.join(folder,'source.rmesh')],signal));
  const source=id%64,appearance=await require('./preview-materials.cjs')(service,material.part,meta,view,signal);
  if(appearance[source]?.texture)result.png=appearance[source].texture.png;
  else{const textures=await require('./preview-textures.cjs')(service,material.part,{vertices:[0,0,0,0,0,0,word,0,0]},{...meta,materials:meta.materials.filter(m=>m.source===source)},folder,signal,'png');if(!textures[source])throw Error('Original material has no texture binding');result.png=(await fs.readFile(path.join(folder,textures[source]))).toString('base64');}
  const png=path.join(folder,'target.png');await fs.writeFile(png,Buffer.from(result.png,'base64'));await require('./service.cjs').run(service.tool('rage-mod-cli'),['--png',png],{signal});
  candidate.textures.set(key,result);return result;
 });}finally{await fs.rm(folder,{recursive:true,force:true});}
},true,candidate);};
