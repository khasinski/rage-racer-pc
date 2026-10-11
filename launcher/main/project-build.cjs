const fs=require('node:fs/promises'),path=require('node:path');
const {createHash,randomUUID}=require('node:crypto');
const sha=bytes=>createHash('sha256').update(bytes).digest('hex');
const sameSource=(a,b)=>a?.region===b?.region&&a?.fingerprint===b?.fingerprint;
function value(v){
  if(typeof v==='boolean')return String(v);
  if(Number.isSafeInteger(v))return String(v);
  if(Array.isArray(v)&&v.every(Number.isSafeInteger))return '['+v.join(', ')+']';
  if(typeof v==='string'&&!/[\x00-\x1f"\\#]/.test(v))return '"'+v+'"';
  throw Error('This value cannot be represented by the car profile format');
}
async function apply(service){
  const {run,atomic,source}=require('./service.cjs');
  const editor=service.editor,snapshot=editor.project.snapshot();
  if(service.state.mods.some(m=>m.enabled))throw Error('Package composition into project builds is not connected yet; no changes were applied');
  if(!sameSource(snapshot.source,await source(service.state.disc)))throw Error('Source archive changed; reopen the project');
  const base=path.join(service.bin,snapshot.source.region==='NTSC-J'?'cars.ntscj.toml':'cars.toml');
  const id=randomUUID(),directory=await service.gameModRoot(),builds=path.join(directory,'builds');
  const existing=await fs.lstat(directory).catch(e=>{if(e.code==='ENOENT')return null;throw e;});
  if(existing){
    if(existing.isSymbolicLink()||!existing.isDirectory())throw Error('Game mod root is not a normal directory');
    const marker=JSON.parse(await fs.readFile(path.join(directory,'.rage-editor.json'),'utf8').catch(e=>{if(e.code==='ENOENT')return '{}';throw e;}));
    if(!['rage-editor','rage-mod-manager'].includes(marker.owner))throw Error('Game mod directory contains unmanaged content; it was not changed');
  }
  const staging=path.join(builds,'.'+id+'.tmp'),final=path.join(builds,id);
  await fs.mkdir(staging,{recursive:true});
  let published=false;
  try{
    const frozenBase=path.join(staging,'base.toml');
    if((await fs.stat(base)).size>1024*1024)throw Error('Base car profile exceeds size limit');
    await fs.copyFile(base,frozenBase);
    const {cars}=JSON.parse(await run(service.tool('rage-mod-cli'),['--car-profile',frozenBase]));
    const groups=new Map();
    for(const c of snapshot.changes){
      if(c.kind!=='car')throw Error(`Building ${c.kind} edits is not connected yet; no changes were applied`);
      const car=cars.find(car=>car.id===c.id);
      if(!car||['id','model','grade','variant','fields'].includes(c.field)||!/^[a-z_]+$/.test(c.field))throw Error('Unknown editable car field: '+c.id+' / '+c.field);
      if(!groups.has(c.id))groups.set(c.id,{car,fields:[]});
      groups.get(c.id).fields.push(`${c.field} = ${value(c.value)}`);
    }
    const overlay=path.join(staging,'delta.toml'),profile=path.join(staging,'cars.toml');
    if(groups.size){
      await fs.writeFile(overlay,[...groups.values()].map(({car,fields})=>`[[cars]]\nid = "${car.id}"\nmodel = ${car.model}\ngrade = ${car.grade}\n${fields.join('\n')}\n`).join('\n'));
      await run(service.tool('rage-mod-cli'),['--compose-car-profile',frozenBase,overlay,profile]);
      await fs.rm(overlay);
    }else await fs.copyFile(frozenBase,profile);
    await fs.rm(frozenBase);
    // Reread the final compiler output through the same parser used by runtime.
    await run(service.tool('rage-mod-cli'),['--car-profile',profile]);
    if(!sameSource(snapshot.source,await source(service.state.disc)))throw Error('Source archive changed during the build');
    const profileHash=sha(await fs.readFile(profile)),manifest=path.join(staging,'mod.toml');
    await fs.writeFile(manifest,'[mod]\nid = "editor-project"\n');
    await run(service.tool('rage-mod-cli'),[manifest]);
    const manifestHash=sha(await fs.readFile(manifest));
    const pointer={format:1,build:id,source:snapshot.source,project:snapshot.id,revision:snapshot.revision,profileHash,manifestHash};
    await fs.writeFile(path.join(staging,'build.json'),JSON.stringify(pointer,null,2));
    await fs.rename(staging,final);
    await editor.project.save(editor.file);
    await atomic(path.join(directory,'.rage-editor.json'),JSON.stringify({format:1,owner:'rage-editor'}));
    await atomic(path.join(directory,'active.json'),JSON.stringify(pointer));published=true;
    return {...editor.project.snapshot(),appliedRevision:snapshot.revision};
  }finally{
    await fs.rm(staging,{recursive:true,force:true});
    if(!published)await fs.rm(final,{recursive:true,force:true});
  }
}
async function activeProfile(service){
  if(!service.state.disc?.archive)return '';
  const {source}=require('./service.cjs'),identity=await source(service.state.disc);
  const {run}=require('./service.cjs'),directory=await service.gameModRoot();
  const result=JSON.parse(await run(service.tool('rage-mod-cli'),['--active-build',directory,identity.region,identity.fingerprint]));
  return result.profile||'';
}
module.exports={apply,activeProfile};
