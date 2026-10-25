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
// Fitment is [frontWidth,frontY,frontZ,frontScale,rearWidth,rearY,rearZ,rearScale,
// bodyY,bodyZ,bodyScale], the order saveCarFitment writes. The car record keeps
// the front axle relative to the rear one.
function axleDeltas(fitment){
 const [frontWidth,frontY,frontZ,,,rearY,rearZ,rearScalePct]=fitment;
 return {dx:Math.round(frontWidth/4),dy:frontY-rearY,dz:frontZ-rearZ,rearScalePct};
}
async function apply(service,activeCustomBank){
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
    const {cars,changes:rawChanges}=await require('./car-base.cjs')(service,{root:staging,files},undefined,frozenBase);
    const groups=new Map(),models={},textures={},materials={},audio={};
    for(const [id,fields]of rawChanges)groups.set(id,{car:cars.find(c=>c.id===id),fields:new Map([...fields].map(([key,v])=>[key,value(v)]))});
    await require('./track-edits.cjs').apply(service,snapshot.changes,staging,files);
    Object.assign(audio,await require('./audio.cjs').apply(service,snapshot.changes,staging,files));
    const isCustomChange=c=>{
      if(c.kind!=='car')return false;
      if(c.id.startsWith('custom_car_'))return true;
      const pm=/^car\.player\.(\d+)\.part\./.exec(c.id);
      if(pm&&Number(pm[1])>=200)return true;
      const mm=/^car\.(\d+)\.material\./.exec(c.id);
      if(mm&&Number(mm[1])>=32)return true;
      return false;
    };
    let effectiveChanges=snapshot.changes.filter(c=>!isCustomChange(c));
    if(Number.isInteger(activeCustomBank)&&activeCustomBank>=200&&activeCustomBank%2===0){
      const rawDonor=Number(snapshot.changes.find(x=>x.kind==='car'&&x.id===`custom_car_${activeCustomBank}`&&x.field==='donor_bank')?.value)||10;
      const donorBank=rawDonor>=10&&rawDonor<=72&&rawDonor%2===0?rawDonor:10;
      const donorSlot=(donorBank-10)/2,customSlot=(activeCustomBank-10)/2;
      const donorCar=cars.find(c=>c.variant===donorSlot);
      const donorPartPrefix=`car.player.${donorBank}.part.`,donorMatPrefix=`car.${donorSlot}.material.`;
      effectiveChanges=effectiveChanges.filter(c=>!(c.kind==='car'&&(c.id===donorCar?.id||c.id.startsWith(donorPartPrefix)||c.id.startsWith(donorMatPrefix))));
      const customPartPrefix=`car.player.${activeCustomBank}.part.`,customMatPrefix=`car.${customSlot}.material.`;
      for(const c of snapshot.changes){
        if(c.kind!=='car')continue;
        if(c.id===`custom_car_${activeCustomBank}`&&c.field!=='donor_bank'&&donorCar){
          effectiveChanges.push({kind:'car',id:donorCar.id,field:c.field,value:c.value});
        }else if(c.id.startsWith(customPartPrefix)){
          effectiveChanges.push({kind:'car',id:donorPartPrefix+c.id.slice(customPartPrefix.length),field:c.field,value:c.value});
        }else if(c.id.startsWith(customMatPrefix)){
          effectiveChanges.push({kind:'car',id:donorMatPrefix+c.id.slice(customMatPrefix.length),field:c.field,value:c.value});
        }
      }
    }
    for(const c of effectiveChanges){
      if(c.kind==='track'&&!['mesh','texture','material'].includes(c.field))continue;
      if(c.kind==='audio')continue;
      if(c.kind==='car'&&c.field==='base_mesh')continue;
      if(c.kind==='car'&&c.field==='fitment'){
        const m=/^car\.(player|rival)\.(\d+)\.part\.(\d+)$/.exec(c.id);
        if(!m||!Array.isArray(c.value)||c.value.length!==11)continue;
        const {dx,dy,dz,rearScalePct}=axleDeltas(c.value);
        if(dx||dy||dz||rearScalePct!==100){
          const bank=Number(m[2]);
          const relative=`raw/asset_${String(bank).padStart(3,'0')}.bin`,target=path.join(staging,relative);
          const src=files.includes(relative)?target:path.join(service.state.disc.data,relative);
          const rawBytes=await fs.readFile(src);
          const clamp16=v=>Math.max(-32768,Math.min(32767,Math.round(v)));
          const positions=[];
          if(m[1]==='player'&&rawBytes.length>=40){
            positions.push(0);
          }else if(m[1]==='rival'&&rawBytes.length>=44){
            const indices=[[0],[1],[2],[3],[4,5],[6,7],[8,9,10]][Math.floor(Number(m[3])/5)]||[];
            const off0=rawBytes.readUInt32LE(0);
            for(const modelIdx of indices){
              if(off0>=44&&off0+12+(modelIdx+1)*8<=rawBytes.length)positions.push(off0+12+modelIdx*8);
            }
          }
          if(positions.length){
            for(const pos of positions){
              if(dx)rawBytes.writeInt16LE(clamp16(rawBytes.readInt16LE(pos)-dx),pos);
              if(dy)rawBytes.writeInt16LE(clamp16(rawBytes.readInt16LE(pos+2)-dy/4),pos+2);
              if(dz)rawBytes.writeInt16LE(clamp16(rawBytes.readInt16LE(pos+4)-dz/4),pos+4);
              if(rearScalePct!==100)rawBytes.writeInt16LE(clamp16(rawBytes.readInt16LE(pos+6)*(rearScalePct/100)),pos+6);
            }
            await fs.mkdir(path.dirname(target),{recursive:true});
            await fs.writeFile(target,rawBytes);
            if(!files.includes(relative))files.push(relative);
          }
        }
        continue;
      }
      if(c.kind!=='car'&&!(c.kind==='track'&&['mesh','texture','material'].includes(c.field))&&!(c.kind==='interface'&&c.field==='texture'))throw Error(`Building ${c.kind} edits is not connected yet; no changes were applied`);
      if(c.field==='material'){
        materials[c.id]=c.value;continue; // Native profile writer validates both ID and values.
      }
      if(c.field==='mesh'||c.field==='texture'){
        const mesh=c.field==='mesh';
        if(c.kind==='interface'?!/^ui\.title\.(grand-prix|extra-grand-prix|time-attack|custom|load-save|options)(\.selected)?$/.test(c.id):mesh?!(/^(car\.(player|rival)\.\d+\.part\.\d+|track\.(?:(?:big|mid|hi|oval)[1-6]|custom(?:[1-9]|[12][0-9]|3[0-2]))\.(course|terrain))$/.test(c.id)):! /^(car\.\d+|track\.(?:(?:big|mid|hi|oval)[1-6]|custom(?:[1-9]|[12][0-9]|3[0-2]))\.(model-bank-1|course|terrain))\.material\.\d+$/.test(c.id))throw Error('Invalid car asset slot');
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
      if(!groups.has(c.id))groups.set(c.id,{car,fields:new Map()});
      groups.get(c.id).fields.set(c.field,value(c.value));
    }
    const overlay=path.join(staging,'delta.toml'),profile=path.join(staging,'cars.toml');
    if(groups.size){
      await fs.writeFile(overlay,[...groups.values()].map(({car,fields})=>`[[cars]]\nid = "${car.id}"\nmodel = ${car.model}\ngrade = ${car.grade}\n${[...fields].map(([key,v])=>key+' = '+v).join('\n')}\n`).join('\n'));
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
    if(Object.keys(models).length||Object.keys(textures).length||Object.keys(materials).length||Object.keys(audio).length){
      const resolvedTextures={...previous.textures,...textures},resolvedMaterials={...previous.materials,...materials};
      const resolvedMeshes={...previous.meshes,...models};
      // Player car banks use even wheel submeshes (2,4..20) for front wheels and odd (3,5..21) for rear wheels across LODs/rotation phases.
      for(const [key,val]of Object.entries(resolvedMeshes)){
        const m=/^car\.player\.(\d+)\.part\.(2|3)$/.exec(key);
        if(m){
          const bank=m[1],baseWheel=Number(m[2]);
          for(let w=baseWheel+2;w<=21;w+=2){
            const wKey=`car.player.${bank}.part.${w}`;
            if(!Object.hasOwn(models,wKey))resolvedMeshes[wKey]=val;
          }
        }
      }
      // A project track-surface override applies to every package variant.
      for(const [overrides,resolved]of [[textures,resolvedTextures],[materials,resolvedMaterials]])for(const [id,file]of Object.entries(overrides))if(/^(car\.\d+|track\.(?:(?:big|mid|hi|oval)[1-6]|custom(?:[1-9]|[12][0-9]|3[0-2]))\.(course|terrain|model-bank-1))\.material\.\d+$/.test(id))for(const key of Object.keys(resolved))if(key.startsWith(id+'.variant.')&&!Object.hasOwn(overrides,key))resolved[key]=file;
      await fs.rm(manifest);
      await run(service.tool('rage-mod-cli'),['--write-profile-stdin',manifest],{
        input:Buffer.from(JSON.stringify({textures:resolvedTextures,materials:resolvedMaterials,meshes:resolvedMeshes,audio:{...previous.audio,...audio}}))});
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
  return result.active?{...result,directory:path.normalize(result.directory),profile:path.normalize(result.profile)}:null;
}
async function activeProfile(service){return (await activeBuild(service))?.profile||'';}
module.exports={apply,activeBuild,activeProfile,axleDeltas};
