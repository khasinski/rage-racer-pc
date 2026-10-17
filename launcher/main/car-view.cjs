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
function applyFitment(bytes,role,fitArr){
 if(!Array.isArray(fitArr)||fitArr.length!==11)return bytes;
 const [frontWidth,frontY,frontZ,frontScalePct,rearWidth,rearY,rearZ,rearScalePct,bodyY,bodyZ,bodyScalePct]=fitArr;
 const bodyScale=bodyScalePct/100,frontScale=frontScalePct/100,rearScale=rearScalePct/100;
 if(bytes.length<24||bytes.subarray(0,7).toString('ascii')!=='RRMESH1')throw Error('Invalid car RRMESH header');
 const meshCount=bytes.readUInt32LE(12),vertexCount=bytes.readUInt32LE(16);
 const vBase=24+(meshCount+1)*4;
 if(vBase+vertexCount*40>bytes.length)throw Error('Truncated car RRMESH vertex buffer');
 const bbox=pred=>{
  let minX=Infinity,minY=Infinity,minZ=Infinity,maxX=-Infinity,maxY=-Infinity,maxZ=-Infinity,cnt=0;
  for(let i=0;i<vertexCount;i++){
   const at=vBase+i*40,x=bytes.readFloatLE(at),y=bytes.readFloatLE(at+4),z=bytes.readFloatLE(at+8);
   if(pred&&!pred(x,y,z))continue;
   minX=Math.min(minX,x);maxX=Math.max(maxX,x);
   minY=Math.min(minY,y);maxY=Math.max(maxY,y);
   minZ=Math.min(minZ,z);maxZ=Math.max(maxZ,z);
   cnt++;
  }
  return cnt?[(minX+maxX)/2,(minY+maxY)/2,(minZ+maxZ)/2]:[0,0,0];
 };
 if(role==='body'){
  const [cx,cy,cz]=bbox();
  for(let i=0;i<vertexCount;i++){
   const at=vBase+i*40;
   bytes.writeFloatLE(cx+(bytes.readFloatLE(at)-cx)*bodyScale,at);
   bytes.writeFloatLE(cy+(bytes.readFloatLE(at+4)-cy)*bodyScale+(bodyY-rearY),at+4);
   bytes.writeFloatLE(cz+(bytes.readFloatLE(at+8)-cz)*bodyScale+(bodyZ-rearZ),at+8);
  }
 }else if(role==='front'){
  const [cx,cy,cz]=bbox();
  for(let i=0;i<vertexCount;i++){
   const at=vBase+i*40;
   bytes.writeFloatLE(cx+(bytes.readFloatLE(at)-cx)*frontScale+frontWidth,at);
   bytes.writeFloatLE(cy+(bytes.readFloatLE(at+4)-cy)*frontScale,at+4);
   bytes.writeFloatLE(cz+(bytes.readFloatLE(at+8)-cz)*frontScale,at+8);
  }
 }else if(role==='rear'){
  const [cxL,cyL,czL]=bbox(x=>x<0);
  const [cxR,cyR,czR]=bbox(x=>x>=0);
  for(let i=0;i<vertexCount;i++){
   const at=vBase+i*40,x=bytes.readFloatLE(at),y=bytes.readFloatLE(at+4),z=bytes.readFloatLE(at+8);
   if(x<0){
    bytes.writeFloatLE(cxL+(x-cxL)*rearScale-rearWidth,at);
    bytes.writeFloatLE(cyL+(y-cyL)*rearScale,at+4);
    bytes.writeFloatLE(czL+(z-czL)*rearScale,at+8);
   }else{
    bytes.writeFloatLE(cxR+(x-cxR)*rearScale+rearWidth,at);
    bytes.writeFloatLE(cyR+(y-cyR)*rearScale,at+4);
    bytes.writeFloatLE(czR+(z-czR)*rearScale,at+8);
   }
  }
 }
 return bytes;
}
async function model(service,key,target,view,forBinding=false){
 const relative=view?.manifest.meshes?.[key];
 if(relative){
  if(!view.files.includes(relative)||!/^meshes\/[A-Za-z0-9_./-]+\.rmesh$/.test(relative)||relative.split('/').some(p=>!p||p==='.'||p==='..'))throw Error('Unknown mod model');
  await fs.copyFile(path.join(view.root||path.join(service.root,'mods',view.id),relative),target);
 }
 if(view?.project){
  const changes=service.editor?.project.snapshot().changes||[];
  const m=/^car\.(player|rival)\.(\d+)\.part\.(\d+)$/.exec(key);
  const bodyPart=m?(m[1]==='rival'?Math.floor(Number(m[3])/5)*5:0):0;
  const bodyKey=m?`car.${m[1]}.${m[2]}.part.${bodyPart}`:null;
  const fitChange=bodyKey?changes.find(c=>c.kind==='car'&&c.id===bodyKey&&c.field==='fitment'):null;
  const baseChange=changes.find(c=>c.kind==='car'&&c.id===key&&c.field==='base_mesh');
  const meshChange=changes.find(c=>c.kind==='car'&&c.id===key&&c.field==='mesh');
  if(forBinding){
   const change=baseChange||(fitChange?null:meshChange);
   if(change)await fs.writeFile(target,await new ProjectFiles(service.editor.file).read(change.value));
   return;
  }
  if(fitChange&&!baseChange&&m){
   const offset=Number(m[3])-bodyPart;
   const role=offset===0?'body':offset===2?'front':offset===3?'rear':null;
   const exists=role&&await fs.stat(target).catch(e=>{if(e.code==='ENOENT')return null;throw e;});
   if(exists){
    const bytes=Buffer.from(await fs.readFile(target));
    await fs.writeFile(target,applyFitment(bytes,role,fitChange.value));
    return;
   }
  }
  if(meshChange)await fs.writeFile(target,await new ProjectFiles(service.editor.file).read(meshChange.value));
 }
}
function resolveBank(service,bank){
 const n=Number(bank);
 if(Number.isInteger(n)&&n>=200){
  const base=n-(n%2);
  return service.resolvePlayerBank(base)+(n%2);
 }
 return n;
}
function rawSource(service,bank,view){
 const actual=resolveBank(service,bank);
 const relative='raw/asset_'+String(actual).padStart(3,'0')+'.bin';
 return path.join(view?.files.includes(relative)?view.root||path.join(service.root,'mods',view.id):service.state.disc.data,relative);
}
async function carMaterials(service,flag,kind,bank,view,signal){
 const {run}=require('./service.cjs'),bankNum=Number(bank),actualBank=resolveBank(service,bankNum);
 const table=JSON.parse(await run(service.tool('rage-mod-cli'),[flag,kind,rawSource(service,actualBank,view),String(actualBank)],{signal}));
 if(kind==='player'&&bankNum>=200&&Array.isArray(table.materials)){
  const fromPrefix=`car.${(actualBank-10)/2}.material.`,toPrefix=`car.${(bankNum-10)/2}.material.`;
  for(const m of table.materials)if(typeof m.id==='string'&&m.id.startsWith(fromPrefix))m.id=toPrefix+m.id.slice(fromPrefix.length);
 }
 return table;
}
async function exportPart(service,key,target,view,signal,binding){
 if(view===undefined)return withView(service,signal,undefined,selected=>exportPart(service,key,target,selected,signal,binding));
 const match=/^car\.(player|rival)\.(\d+)\.part\.(\d+)$/.exec(key);if(!match)throw Error('Invalid car part');
 const bankNum=Number(match[2]),actualBank=String(resolveBank(service,bankNum));
 const exportKey=bankNum>=200?`car.${match[1]}.${actualBank}.part.${match[3]}`:key;
 const relative='raw/asset_'+actualBank.padStart(3,'0')+'.bin';
 let raw=view?.files.includes(relative);
 if(raw){view.geometry ||= new Map();if(!view.geometry.has(actualBank))view.geometry.set(actualBank,!JSON.parse(await require('./service.cjs').run(service.tool('rage-mod-cli'),['--car-bank-equal',match[1],rawSource(service,actualBank),rawSource(service,actualBank,view)],{signal})).equal);raw=view.geometry.get(actualBank);}
 if(binding===undefined){
  const override=target+'.bindings';
  try{
   await model(service,key,override,view,true);
   const exists=await fs.stat(override).catch(e=>{if(e.code==='ENOENT')return null;throw e;});
   if(exists)binding=JSON.parse(await require('./service.cjs').run(service.tool('rage-mod-cli'),['--mesh',override],{signal})).bankBindings;
  }finally{await fs.rm(override,{force:true});}
 }
 if(binding===-1)throw Error('Car model mixes original-bank and built-in material bindings');
 raw = binding!==undefined ? binding===1 || raw : raw || service.state.settings?.['modern.authored_cars']==='0';
 if(!raw){view.authored ??= JSON.parse(await service.carTool(['--set','tools.car_catalog=true'],signal)).parts.length>0;raw=!view.authored;}
 const args=raw?['--set','tools.car_raw='+rawSource(service,actualBank,view),'--set','tools.car_bank='+actualBank,'--set','tools.car_rival='+(match[1]==='rival'),'--set','tools.car_part='+match[3],'--set','tools.car_output='+target]:['--set','tools.car_export='+exportKey,'--set','tools.car_output='+target];
 return JSON.parse(await service.carTool(args,signal));
}
module.exports={withView,model,resolveBank,rawSource,carMaterials,exportPart,applyFitment};
