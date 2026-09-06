const fs=require('node:fs/promises'),path=require('node:path');
module.exports=async function validateBindings(service,key,mesh,staging,label,signal){
 const {run}=require('./service.cjs'),base=path.join(staging,'base-materials.rmesh');
 try{
  const metadata=JSON.parse(await service.carTool(['--set','tools.car_export='+key,'--set','tools.car_output='+base],signal));
  if(!Array.isArray(metadata.materials))throw Error('Could not read the original car material bindings');
  const available=new Set(metadata.materials.map(m=>m.source));
  const stats=JSON.parse(await run(service.tool('rage-mod-cli'),['--mesh',mesh],{signal}));
  for(const material of stats.materials){
   if(material===65535)continue; // Deliberately untextured, e.g. inner wheel wells.
   const source=material%64;
   if(material>=6*64)throw Error(`${label} uses an unsupported car surface. Preserve the exported rage_* material names.`);
   if(!available.has(source))throw Error(`${label} references texture source ${source}, which is unavailable in this car part. Preserve the exported rage_* material names.`);
  }
 }finally{await fs.rm(base,{force:true});}
};
