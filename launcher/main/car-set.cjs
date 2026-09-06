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
exports.exportSet=async(service,key,destination)=>{
 service.requireReady();return service.operation('Exporting complete car',async signal=>{
  const c=await car(service,key,signal),materials=await table(service,c,signal),target=path.join(destination,'car-set-'+randomUUID());await fs.mkdir(target);
  try{
   const manifest={format:1,region:service.state.disc.region,key,parts:{},textures:{}};await fs.mkdir(path.join(target,'textures'));
   for(const [role,offset]of [['body',0],['front-wheels',2],['rear-wheels',3]]){
    const partKey=`car.${c.rival?'rival':'player'}.${c.bank}.part.${c.part+offset}`;
    const folder=await service.exportCarData(partKey,target,signal,'png'),named=path.join(target,role);await fs.rename(folder,named);
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
   await fs.writeFile(path.join(target,'README.txt'),'Edit the three OBJ files and the shared PNG textures, then Import car set on the same slot.\nKeep rage_* material names, UVs and normals. Export triangulated OBJ.\nShared PNG files are referenced by body and wheel materials; edit each shared image once.\ncar-set.json maps the files to this disc version and car slot. Other slots require material remapping.\n');
   return target;
  }catch(e){await fs.rm(target,{recursive:true,force:true});throw e;}
 });
};
exports.importSet=async(service,key,source)=>{
 service.requireReady();return service.operation('Importing complete car',async signal=>{
  const {run}=require('./service.cjs'),c=await car(service,key,signal);
  const manifest=JSON.parse(await fs.readFile(await boundedFile(source,'car-set.json',65536),'utf8'));
  if(manifest.format!==1||manifest.region!==service.state.disc.region||manifest.key!==key)throw Error('Choose a car set exported for this slot and game version. Cross-slot material remapping is not supported yet.');
  if(!manifest.parts||!manifest.textures||Array.isArray(manifest.textures)||Object.keys(manifest.textures).length>64)throw Error('Invalid car set manifest');
  const materials=await table(service,c,signal),staging=path.join(service.root,'car-set-import-'+randomUUID());await fs.mkdir(path.join(staging,'meshes'),{recursive:true});await fs.mkdir(path.join(staging,'textures'));
  try{
   const meshes=[],textures=[];
   for(const [role,offset]of [['body',0],['front-wheels',2],['rear-wheels',3]]){
    const file=await boundedFile(source,manifest.parts[role],128*1024*1024),ext=path.extname(file).toLowerCase(),relative='meshes/'+role+'.rmesh',out=path.join(staging,relative);
    if(ext==='.obj')await run(service.tool('rage-mesh-obj'),['import',file,out],{signal});else if(ext==='.rmesh')await fs.copyFile(file,out);else throw Error('Car set models must be OBJ or RRMESH');
    const partKey=`car.${c.rival?'rival':'player'}.${c.bank}.part.${c.part+offset}`;
    await require('./car-material-bindings.cjs')(service,partKey,out,staging,role,signal);
    meshes.push(`"${partKey}" = "${relative}"`);
   }
   for(const [slot,relative]of Object.entries(manifest.textures)){
    const material=materials.find(m=>String(m.slot)===slot);if(!material)throw Error('Unknown car texture slot');
    const file=await boundedFile(source,relative,32*1024*1024),bytes=await fs.readFile(file);
    if(bytes.length<24||!bytes.subarray(0,8).equals(Buffer.from([137,80,78,71,13,10,26,10]))||!bytes.readUInt32BE(16)||!bytes.readUInt32BE(20)||bytes.readUInt32BE(16)>4096||bytes.readUInt32BE(20)>4096)throw Error('Car textures must be PNG images up to 4096 × 4096');
    const output='textures/material-'+slot+'.png',staged=path.join(staging,output);await fs.copyFile(file,staged);
    try{await run(service.tool('rage-mod-cli'),['--png',staged],{signal});}catch(e){if(signal.aborted)throw e;throw Error(`Cannot import texture ${relative}: the game could not decode this PNG.`);}
    textures.push(`"${material.id}" = "${output}"`);
   }
   await fs.writeFile(path.join(staging,'mod.toml'),'[meshes]\n'+meshes.join('\n')+'\n[textures]\n'+textures.join('\n')+'\n');
   const names=JSON.parse(await run(service.tool('rage-save-cli'),['car-names'],{signal})),name=require('./car-names.cjs')(c.name,service.state.disc.region,names);
   return (await service.installModFiles(staging,signal,name+' · complete car')).id;
  }finally{await fs.rm(staging,{recursive:true,force:true});}
 });
};
