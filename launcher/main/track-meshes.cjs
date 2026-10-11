const fs=require('node:fs/promises'),path=require('node:path');
const {ProjectFiles}=require('./project-files.cjs');
const pattern=/^track\.(big|mid|hi|oval)([1-6])\.(course|terrain)$/;
function slot(id){
 const m=pattern.exec(id);if(!m)throw Error('Invalid track mesh slot');
 return {bank:88+(Number(m[2])-1)*8+['big','mid','hi','oval'].indexOf(m[1])*2,name:m[3]};
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
   for(const {name,relative}of entries){const info=JSON.parse(await run(service.tool('rage-mod-cli'),['--mesh-bank',path.join(directory,relative)],{signal}));if(info.meshes!==metadata[name].meshes)throw Error('Track model bank must retain its model count and order');}
  }finally{await fs.rm(folder,{recursive:true,force:true});}
 }
}
module.exports.validateBuild=validateBuild;
