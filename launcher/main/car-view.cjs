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
module.exports={withView,model};
