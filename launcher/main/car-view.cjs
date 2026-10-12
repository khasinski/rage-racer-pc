const fs=require('node:fs/promises'),path=require('node:path');
const {ProjectFiles}=require('./project-files.cjs');

// A single immutable package composition is shared by every part in one
// preview/export. Explicit package comparison does not include project edits.
async function withView(service,signal,selected,work){
 if(selected!==undefined)return work(selected);
 await service.loadProject();
 const selection=service.editor?.project.snapshot().selection;
 let root='';
 try{
  if(selection?.packages.length)root=await service.composeMods(service.modsForSelection(selection),selection.resolutions);
  const {run}=require('./service.cjs');
  const view=root?{root,project:true,files:await require('./mods.cjs').inventory(root,service.tool('rage-mod-cli')),manifest:JSON.parse(await run(service.tool('rage-mod-cli'),[path.join(root,'mod.toml')],{signal}))}:{project:true,files:[],manifest:{}};
  return await work(view);
 }finally{if(root)await fs.rm(root,{recursive:true,force:true});}
}
async function model(service,key,target,view){
 const relative=view?.manifest.meshes?.[key];
 if(relative){
  if(!view.files.includes(relative)||!/^meshes\/[A-Za-z0-9_./-]+\.rmesh$/.test(relative)||relative.split('/').some(p=>!p||p==='.'||p==='..'))throw Error('Unknown mod model');
  await fs.copyFile(path.join(view.root||path.join(service.root,'mods',view.id),relative),target);
 }
 if(view?.project){
  const change=service.editor?.project.snapshot().changes.find(c=>c.kind==='car'&&c.id===key&&c.field==='mesh');
  if(change)await fs.writeFile(target,await new ProjectFiles(service.editor.file).read(change.value));
 }
}
function rawSource(service,bank,view){const relative='raw/asset_'+String(bank).padStart(3,'0')+'.bin';return path.join(view?.files.includes(relative)?view.root||path.join(service.root,'mods',view.id):service.state.disc.data,relative);}
async function exportPart(service,key,target,view,signal,binding){
 if(view===undefined)return withView(service,signal,undefined,selected=>exportPart(service,key,target,selected,signal,binding));
 const match=/^car\.(player|rival)\.(\d+)\.part\.(\d+)$/.exec(key);if(!match)throw Error('Invalid car part');
 const relative='raw/asset_'+match[2].padStart(3,'0')+'.bin';
 let raw=view?.files.includes(relative);
 if(raw){view.geometry ||= new Map();if(!view.geometry.has(match[2]))view.geometry.set(match[2],!JSON.parse(await require('./service.cjs').run(service.tool('rage-mod-cli'),['--car-bank-equal',match[1],rawSource(service,match[2]),rawSource(service,match[2],view)],{signal})).equal);raw=view.geometry.get(match[2]);}
 if(binding===undefined){
  const override=target+'.bindings';
  try{
   await model(service,key,override,view);
   const exists=await fs.stat(override).catch(e=>{if(e.code==='ENOENT')return null;throw e;});
   if(exists)binding=JSON.parse(await require('./service.cjs').run(service.tool('rage-mod-cli'),['--mesh',override],{signal})).bankBindings;
  }finally{await fs.rm(override,{force:true});}
 }
 if(binding===-1)throw Error('Car model mixes original-bank and built-in material bindings');
 raw = binding!==undefined ? binding===1 || raw : raw || service.state.settings?.['modern.authored_cars']==='0';
 const args=raw?['--set','tools.car_raw='+rawSource(service,match[2],view),'--set','tools.car_bank='+match[2],'--set','tools.car_rival='+(match[1]==='rival'),'--set','tools.car_part='+match[3],'--set','tools.car_output='+target]:['--set','tools.car_export='+key,'--set','tools.car_output='+target];
 return JSON.parse(await service.carTool(args,signal));
}
module.exports={withView,model,rawSource,exportPart};
