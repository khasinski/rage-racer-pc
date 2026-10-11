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
  if(!sameSource(snapshot.source,await source(service.state.disc)))throw Error('Source archive changed; reopen the project');
  const base=path.join(service.bin,snapshot.source.region==='NTSC-J'?'cars.ntscj.toml':'cars.toml');
  const id=randomUUID(),directory=await service.gameModRoot(),builds=path.join(directory,'builds');
  const existing=await fs.lstat(directory).catch(e=>{if(e.code==='ENOENT')return null;throw e;});
  if(existing){
    if(existing.isSymbolicLink()||!existing.isDirectory())throw Error('Game mod root is not a normal directory');
    const marker=JSON.parse(await fs.readFile(path.join(directory,'.rage-editor.json'),'utf8').catch(e=>{if(e.code==='ENOENT')return '{}';throw e;}));
    if(marker.format!==1||!['rage-editor','rage-mod-manager'].includes(marker.owner))throw Error('Game mod directory contains unmanaged content; it was not changed');
  }
  const staging=path.join(builds,'.'+id+'.tmp'),final=path.join(builds,id);
  await fs.mkdir(staging,{recursive:true});
  let published=false,composed='';
  try{
    composed=await service.composeMods(service.modsForSelection(snapshot.selection),snapshot.selection.resolutions);
    let files=[];
    if(composed){
      files=await require('./mods.cjs').inventory(composed,service.tool('rage-mod-cli'));
      await service.snapshotModFiles(composed,staging,files);
    }
    const frozenBase=path.join(staging,'base.toml');
    if((await fs.stat(base)).size>1024*1024)throw Error('Base car profile exceeds size limit');
    await fs.copyFile(base,frozenBase);
    const {cars}=JSON.parse(await run(service.tool('rage-mod-cli'),['--car-profile',frozenBase]));
    const groups=new Map(),models={},textures={},materials={};
    await require('./track-edits.cjs').apply(service,snapshot.changes,staging,files);
    for(const c of snapshot.changes){
      if(c.kind==='track'&&!['mesh','texture','material'].includes(c.field))continue;
      if(c.kind!=='car'&&!(c.kind==='track'&&['mesh','texture','material'].includes(c.field))&&!(c.kind==='interface'&&c.field==='texture'))throw Error(`Building ${c.kind} edits is not connected yet; no changes were applied`);
      if(c.field==='material'){
        materials[c.id]=c.value;continue; // Native profile writer validates both ID and values.
      }
      if(c.field==='mesh'||c.field==='texture'){
        const mesh=c.field==='mesh';
        if(c.kind==='interface'?!/^ui\.title\.(grand-prix|extra-grand-prix|time-attack|custom|load-save|options)(\.selected)?$/.test(c.id):mesh?!(/^(car\.(player|rival)\.\d+\.part\.\d+|track\.(big|mid|hi|oval)[1-6]\.(course|terrain))$/.test(c.id)):! /^(car\.\d+|track\.(big|mid|hi|oval)[1-6]\.(model-bank-1|course|terrain))\.material\.\d+$/.test(c.id))throw Error('Invalid car asset slot');
        if(!c.value.endsWith(mesh?'.rmesh':'.png'))throw Error('Invalid project asset type');
        const bytes=await new (require('./project-files.cjs').ProjectFiles)(editor.file).read(c.value);
        const relative=(mesh?'meshes':'textures')+'/project/'+c.value.slice(7);
        await fs.mkdir(path.dirname(path.join(staging,relative)),{recursive:true});
        await fs.writeFile(path.join(staging,relative),bytes);
        const checked=await run(service.tool('rage-mod-cli'),[mesh?(c.kind==='track'?'--mesh-bank':'--mesh'):'--png',path.join(staging,relative)]);
        if(c.kind==='interface'){const png=JSON.parse(checked);if(png.width!==112||png.height!==16)throw Error('Title menu images must be 112×16 PNGs');}
        (mesh?models:textures)[c.id]=relative;if(!files.includes(relative))files.push(relative);
        continue;
      }
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
    if(!composed)await fs.writeFile(manifest,'[mod]\nid = "editor-project"\n');
    const previous=JSON.parse(await run(service.tool('rage-mod-cli'),[manifest]));
    await require('./track-meshes.cjs').validateBuild(service,{...previous.meshes,...models},staging,files);
    if(Object.keys(models).length||Object.keys(textures).length||Object.keys(materials).length){
      await fs.rm(manifest);
      await run(service.tool('rage-mod-cli'),['--write-profile-stdin',manifest],{
        input:Buffer.from(JSON.stringify({textures:{...previous.textures,...textures},materials:{...previous.materials,...materials},meshes:{...previous.meshes,...models}}))});
    }
    await run(service.tool('rage-mod-cli'),[manifest]);
    const manifestHash=sha(await fs.readFile(manifest));
    const payload=JSON.parse(await run(service.tool('rage-mod-cli'),['--hash-files-stdin',staging],{
      input:Buffer.from(JSON.stringify(files.filter(name=>name!=='mod.toml').sort())),maxOutput:8*1024*1024}));
    const payloadBytes=JSON.stringify(payload);
    await fs.writeFile(path.join(staging,'files.json'),payloadBytes);
    const pointer={format:2,build:id,source:snapshot.source,project:snapshot.id,revision:snapshot.revision,profileHash,manifestHash,payloadHash:sha(payloadBytes)};
    await fs.writeFile(path.join(staging,'build.json'),JSON.stringify(pointer,null,2));
    await fs.rename(staging,final);
    await run(service.tool('rage-mod-cli'),['--check-build',directory,path.join(final,'build.json'),snapshot.source.region,snapshot.source.fingerprint]);
    if(!sameSource(snapshot.source,await source(service.state.disc)))throw Error('Source archive changed during the build');
    await editor.project.save(editor.file);
    await atomic(path.join(directory,'.rage-editor.json'),JSON.stringify({format:1,owner:'rage-editor'}));
    await atomic(path.join(directory,'active.json'),JSON.stringify(pointer));published=true;
    return {...editor.project.snapshot(),appliedRevision:snapshot.revision};
  }finally{
    if(composed)await fs.rm(composed,{recursive:true,force:true});
    await fs.rm(staging,{recursive:true,force:true});
    if(!published)await fs.rm(final,{recursive:true,force:true});
  }
}
async function activeBuild(service){
  if(!service.state.disc?.archive)return null;
  const {source}=require('./service.cjs'),identity=await source(service.state.disc);
  const {run}=require('./service.cjs'),directory=await service.gameModRoot();
  const result=JSON.parse(await run(service.tool('rage-mod-cli'),['--active-build',directory,identity.region,identity.fingerprint]));
  return result.active?result:null;
}
async function activeProfile(service){return (await activeBuild(service))?.profile||'';}
module.exports={apply,activeBuild,activeProfile};
