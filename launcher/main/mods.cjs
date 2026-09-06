const {readDependencies}=require('./mod-dependencies.cjs');
const fs=require('node:fs/promises');
const path=require('node:path');
const {randomUUID}=require('node:crypto');
// Only data formats understood by the current runtime. No executable hooks.
function allowed(name){const raw=/^raw\/asset_(\d{3})\.bin$/.exec(name);return (raw!==null&&Number(raw[1])<135)||/^textures\/[A-Za-z0-9_./-]+\.(png|json|txt)$/.test(name)||/^meshes\/[A-Za-z0-9_./-]+\.rmesh$/.test(name)||name==='mod.toml'||name==='manifest.json'||name==='rage-mod.json';}
async function inventory(root){
 const result=[];let total=0;
 async function visit(relative){for(const item of await fs.readdir(path.join(root,relative),{withFileTypes:true})){
   if(item.name.startsWith('.'))continue;
   const name=relative?relative+'/'+item.name:item.name;
   if(item.isSymbolicLink())throw Error('Mod folders cannot contain symbolic links');
   if(item.isDirectory()){if(!['raw','textures','meshes'].includes(name.split('/')[0])||name.split('/').length>8)throw Error('Unsupported mod folder: '+name);await visit(name);}
   else {if(!item.isFile()||!allowed(name))throw Error('Unsupported mod file: '+name);const info=await fs.stat(path.join(root,name));total+=info.size;if(info.size>128*1024*1024||total>1024*1024*1024||result.length>=10000)throw Error('Mod package is too large');result.push(name);}
 }}await visit('');if(!result.length)throw Error('The mod folder is empty');return result.sort();
}
async function legacyTextures(root,files){
 if(!files.includes('textures/index.txt'))return [];
 const text=await fs.readFile(path.join(root,'textures/index.txt'),'utf8'),entries=[];
 for(const line of text.split(/\r?\n/)){
  if(!line.trim()||line.trim().startsWith('#'))continue;
  const match=/^\s*(\d+)\s+([A-Za-z0-9_./-]+\.json)\s*$/.exec(line);
  if(!match||Number(match[1])>=135||match[2].length>255||match[2].split('/').some(p=>!p||p==='.'||p==='..'))throw Error('Invalid legacy texture index entry');
  const json='textures/'+match[2],png=json.slice(0,-5)+'.png';
  if(!files.includes(json)||!files.includes(png))throw Error('Missing legacy texture pair: '+match[2]);
  entries.push({asset:Number(match[1]),json,png});
 }
 return entries;
}
function legacyFiles(mod){return new Set((mod.legacyTextures||[]).flatMap(t=>[t.json,t.png]).concat(mod.files.includes('textures/index.txt')?['textures/index.txt']:[]));}
function installMethods(Service){
 Service.prototype.validateModSelection=async function(){
  const active=this.state.mods.filter(m=>m.enabled),args=['--check-selection'];
  if(!active.length)return active;
  for(const mod of active){
   readDependencies(mod);
   args.push('--mod',mod.packageId||mod.id,mod.version||'',mod.region,
    mod.files.includes('mod.toml')?path.join(this.root,'mods',mod.id,'mod.toml'):'');
   for(const dependency of mod.requires||[])args.push('--requires',dependency.packageId,dependency.version||'');
  }
  const {run}=require('./service.cjs');
  const order=JSON.parse(await run(this.tool('rage-mod-cli'),args));
  return order.map(index=>active[index]);
 };
 Service.prototype.refreshLegacyTextures=async function(){
  for(const mod of this.state.mods)mod.legacyTextures=await legacyTextures(path.join(this.root,'mods',mod.id),mod.files);
 };
 Service.prototype.replaceAsset=async function(index,file){
  this.requireReady();if(!Number.isInteger(index)||index<0||index>=135)throw Error('Invalid asset');
  return this.operation('Importing asset replacement',async signal=>{
   const source=path.join(this.root,'asset-replacement-'+randomUUID());
   await fs.mkdir(path.join(source,'raw'),{recursive:true});
   try{await fs.copyFile(file,path.join(source,'raw',`asset_${String(index).padStart(3,'0')}.bin`));const mod=await this.installModFiles(source,signal,`Asset ${index} replacement`);return mod.id;}
   finally{await fs.rm(source,{recursive:true,force:true});}
  });
 };
 Service.prototype.saveMaterial=async function(id,key,properties){
  this.requireReady();if(this.game)throw Error('Close the game before editing mods');
  const mod=this.state.mods.find(m=>m.id===id);if(!mod)throw Error('Unknown mod');
  if(typeof key!=='string'||!/^[a-z0-9.-]+$/.test(key)||key.length>150||typeof properties!=='string'||properties.length>255||/["\\\r\n]/.test(properties))throw Error('Invalid material');
  return this.operation('Saving material',async signal=>{
  const {run}=require('./service.cjs');
  const temp=path.join(this.root,'material-'+randomUUID()+'.toml');
  try{
   const source=mod.files.includes('mod.toml')?path.join(this.root,'mods',id,'mod.toml'):'';
   await run(this.tool('rage-mod-cli'),['--set-material',source,temp,key,properties],{signal});
   const manifest=JSON.parse(await run(this.tool('rage-mod-cli'),[temp],{signal}));
   if(signal.aborted)throw Error('Operation canceled');this.busy.cancellable=false;await this.update();
   const target=path.join(this.root,'mods',id,'mod.toml'),backup=temp+'.previous';
   const previousManifest=mod.manifest,previousFiles=mod.files;
   let backedUp=false,installed=false;
   try{
    try{await fs.rename(target,backup);backedUp=true;}catch(e){if(e.code!=='ENOENT')throw e;}
    await fs.rename(temp,target);installed=true;
    mod.manifest=manifest;mod.files=previousFiles.includes('mod.toml')?previousFiles:[...previousFiles,'mod.toml'];
    await this.persist();
   }catch(e){
    mod.manifest=previousManifest;mod.files=previousFiles;
    if(installed)await fs.rm(target,{force:true});
    if(backedUp)await fs.rename(backup,target);
    throw e;
   }
   if(backedUp)await fs.rm(backup,{force:true});
  }
  finally{await fs.rm(temp,{force:true});}
  });
 };
 Service.prototype.importMod=async function(source){this.requireReady();return this.operation('Importing mod',signal=>this.installModFiles(source,signal));};
 Service.prototype.saveModDetails=async function(id,details){
  this.requireReady();
  const values=require('./mod-details.cjs').validateDetails(details);
  return this.operation('Saving mod details',async()=>{
   const mod=this.state.mods.find(m=>m.id===id);if(!mod)throw Error('Mod not found');
   const previous={...mod};Object.assign(mod,values);
   try{await this.validateModSelection();await this.persist();}catch(e){for(const key of Object.keys(values))if(!Object.hasOwn(previous,key))delete mod[key];Object.assign(mod,previous);throw e;}
  },false);
 };
 Service.prototype.installModFiles=async function(source,signal,name){
   const files=await inventory(source),sourceName=path.basename(source);
   const id=randomUUID(),folder=path.join(this.root,'mods',id);await fs.mkdir(folder,{recursive:true});
   try {
   await this.snapshotModFiles(source,folder,files,signal);
   source=folder;
   const legacy=await legacyTextures(source,files);
   let metadata=null;
   if(files.includes('rage-mod.json')){
    const {run}=require('./service.cjs');
    metadata=JSON.parse(await run(this.tool('rage-mod-cli'),['--metadata',path.join(source,'rage-mod.json')],{signal}));
   }
   let manifest={textures:{},materials:{},meshes:{}};
   if(files.includes('mod.toml')) {
     // The runtime parser is authoritative for material values and semantic IDs.
     const {run}=require('./service.cjs');
     manifest=JSON.parse(await run(this.tool('rage-mod-cli'),[path.join(source,'mod.toml')],{signal}));
     for(const relative of Object.values(manifest.textures))if(!allowed(relative)||!relative.startsWith('textures/'))throw Error('Texture must be under textures/');
   }
   for(const relative of Object.values(manifest.textures))if(!files.includes(relative))throw Error('Missing texture: '+relative);
   for(const [key,relative] of Object.entries(manifest.meshes||{})){
     if(!/^car\.(player|rival)\.\d+\.part\.\d+$/.test(key))throw Error('Unsupported car mesh target: '+key);
     if(!allowed(relative)||!relative.startsWith('meshes/')||!files.includes(relative))throw Error('Missing or invalid mesh: '+relative);
     const {run}=require('./service.cjs');await run(this.tool('rage-mod-cli'),['--mesh',path.join(source,relative)],{signal});
   }
    if(signal.aborted)throw Error('Operation canceled');
    this.busy.cancellable=false;await this.update();
    const mod={id,name:name||metadata?.name||sourceName,files,manifest,legacyTextures:legacy,enabled:false,region:metadata?.region||this.state.disc.region};
    for(const field of ['author','version','description','packageId','requires'])if(metadata?.[field]!==undefined)mod[field]=metadata[field];
    this.state.mods.push(mod);await this.persist();return mod;
   }catch(e){this.state.mods=this.state.mods.filter(m=>m.id!==id);await fs.rm(folder,{recursive:true,force:true});throw e;}
 };
 Service.prototype.snapshotModFiles=async function(source,target,files,signal){
   const pairs=[];
   for(const name of files){
    if(signal?.aborted)throw Error('Operation canceled');
    await fs.mkdir(path.dirname(path.join(target,name)),{recursive:true});
    pairs.push([path.join(source,name),path.join(target,name)]);
   }
   const {run}=require('./service.cjs');
   await run(this.tool('rage-mod-cli'),['--copy-snapshot-stdin'],{input:Buffer.from(JSON.stringify(pairs)),signal});
 };
 Service.prototype.toggleMod=async function(id,enabled){
  this.requireReady();const mod=this.state.mods.find(m=>m.id===id);
  if(!mod||typeof enabled!=='boolean')throw Error('Unknown mod');
  if(enabled&&mod.region!==this.state.disc.region)throw Error('This mod was imported for a different disc region');
  return this.operation('Updating mod',async()=>{
   const previous=mod.enabled;mod.enabled=enabled;
   try{await this.validateModSelection();await this.persist();}catch(e){mod.enabled=previous;throw e;}
  },false);
 };
 Service.prototype.removeMod=async function(id){
  this.requireReady();if(!this.state.mods.some(m=>m.id===id))throw Error('Unknown mod');
  return this.operation('Removing mod',async()=>{
   const previous=this.state.mods;this.state.mods=previous.filter(m=>m.id!==id);
   try{await this.validateModSelection();await this.persist();}catch(e){this.state.mods=previous;throw e;}
   await fs.rm(path.join(this.root,'mods',id),{recursive:true,force:true});
  },false);
 };
 Service.prototype.exportMod=async function(id,destination){
  this.requireReady();const mod=this.state.mods.find(m=>m.id===id);if(!mod)throw Error('Unknown mod');
  return this.operation('Exporting mod',async signal=>{
   const target=path.join(destination,'rage-mod-'+id);await fs.mkdir(target);
   try{
    const source=path.join(this.root,'mods',id);
    for(const name of await inventory(source)){
     if(name==='rage-mod.json')continue;
     if(signal.aborted)throw Error('Operation canceled');
     await fs.mkdir(path.dirname(path.join(target,name)),{recursive:true});
     await fs.copyFile(path.join(source,name),path.join(target,name),require('node:fs').constants.COPYFILE_EXCL);
    }
    if(signal.aborted)throw Error('Operation canceled');
    await fs.writeFile(path.join(target,'rage-mod.json'),JSON.stringify({format:1,name:mod.name,region:mod.region,packageId:mod.packageId||mod.id,requires:mod.requires,author:mod.author,version:mod.version,description:mod.description},null,2)+'\n',{flag:'wx'});
    if(signal.aborted)throw Error('Operation canceled');
    return target;
   }catch(e){await fs.rm(target,{recursive:true,force:true});throw e;}
  });
 };
 Service.prototype.overlaps=function(){
  const owners=new Map();
  for(const mod of this.state.mods.filter(m=>m.enabled)){
   const semanticFiles=new Set([...Object.values(mod.manifest.textures),...Object.values(mod.manifest.meshes||{})]);
   const legacy=legacyFiles(mod);
   const keys=[...(mod.legacyTextures||[]).map(t=>'legacy-textures:asset-'+t.asset),...mod.files.filter(f=>f.startsWith('raw/')||(f.startsWith('textures/')&&!semanticFiles.has(f)&&!legacy.has(f))),...Object.keys(mod.manifest.textures).map(k=>'texture:'+k),...Object.keys(mod.manifest.materials).map(k=>'material:'+k),...Object.keys(mod.manifest.meshes||{}).map(k=>'mesh:'+k)];
   for(const key of new Set(keys)){const list=owners.get(key)||[];list.push({id:mod.id,name:mod.name});owners.set(key,list);}
  }
  return [...owners].filter(([,list])=>list.length>1).map(([key,candidates])=>{
   const choice=this.state.resolutions?.[key];const ids=candidates.map(c=>c.id).sort();
   const valid=choice&&JSON.stringify([...choice.candidates].sort())===JSON.stringify(ids)&&ids.includes(choice.winner);
   return {key,candidates,mods:candidates.map(c=>c.name),winner:valid?choice.winner:null};
  });
 };
 Service.prototype.conflicts=function(){return this.overlaps().filter(c=>!c.winner);};
 Service.prototype.resolveConflict=async function(key,winner){
  this.requireReady();if(this.game)throw Error('Close the game before changing priorities');
  return this.operation('Saving mod priority',async()=>{
  const conflict=this.overlaps().find(c=>c.key===key);
  if(!conflict||!conflict.candidates.some(c=>c.id===winner))throw Error('The conflict changed. Review the current mod list.');
  const previous=this.state.resolutions;
  this.state.resolutions={...previous,[key]:{winner,candidates:conflict.candidates.map(c=>c.id).sort()}};
  try{await this.persist();}catch(e){this.state.resolutions=previous;throw e;}
  },false);
 };
 Service.prototype.composeMods=async function(){
   const active=await this.validateModSelection();if(!active.length)return '';
   // The UI's synchronous conflict view is advisory. The compiled resolver
   // owns final selection and rejects choices made for a different owner set.
   const groups=this.overlaps(),args=['--resolve-providers'];
   for(const group of groups){
    args.push('--resource',group.key);
    for(const candidate of group.candidates)args.push('--candidate',candidate.id);
    const choice=this.state.resolutions?.[group.key];
    if(choice){args.push('--choice',choice.winner);for(const id of choice.candidates)args.push('--previous',id);}
   }
   const {run}=require('./service.cjs');
   if(args.some(token=>typeof token!=='string'||token.includes('\0')))throw Error('Invalid resource conflict input');
   const request=args.length>1?Buffer.from(args.slice(1).join('\0')+'\0','utf8'):Buffer.alloc(0);
   const selectedIndices=JSON.parse(await run(this.tool('rage-mod-cli'),['--resolve-providers-stdin'],{input:request}));
   const winners=new Map(groups.map((group,i)=>[group.key,group.candidates[selectedIndices[i]].id]));
   const selected=(key,id)=>!winners.has(key)||winners.get(key)===id;
   const target=path.join(this.root,'active-mods-'+randomUUID());await fs.mkdir(target,{recursive:true});
   const tables={textures:{},materials:{},meshes:{}},legacyIndex=[];
   try {for(const mod of active){
     const source=path.join(this.root,'mods',mod.id);const files=await inventory(source);
     const semanticFiles=new Set([...Object.values(mod.manifest.textures),...Object.values(mod.manifest.meshes||{})]);
     const legacy=legacyFiles(mod);
     for(const [index,entry]of (mod.legacyTextures||[]).entries())if(selected('legacy-textures:asset-'+entry.asset,mod.id)){
      const stem=`legacy-${mod.id}-${index}`;
      await fs.mkdir(path.join(target,'textures'),{recursive:true});
      await fs.copyFile(path.join(source,entry.json),path.join(target,'textures',stem+'.json'));
      await fs.copyFile(path.join(source,entry.png),path.join(target,'textures',stem+'.png'));
      legacyIndex.push(`${entry.asset} ${stem}.json`);
     }
     for(const name of files){
      if(name==='mod.toml'||name==='manifest.json')continue;
      // Semantic textures use a provider-specific path. Choosing a material or
      // texture ID cannot accidentally select pixels from another mod that used
      // the same source filename.
      if(semanticFiles.has(name)){
       const group=name.split('/')[0];const isolated=group+'/provider-'+mod.id+'/'+name.slice(group.length+1);
       await fs.mkdir(path.dirname(path.join(target,isolated)),{recursive:true});
       await fs.copyFile(path.join(source,name),path.join(target,isolated));
      }
      if(!semanticFiles.has(name)&&!legacy.has(name)&&selected(name,mod.id)){
       await fs.mkdir(path.dirname(path.join(target,name)),{recursive:true});
       await fs.copyFile(path.join(source,name),path.join(target,name));
      }
     }
     for(const[key,value]of Object.entries(mod.manifest.textures))if(selected('texture:'+key,mod.id))tables.textures[key]='textures/provider-'+mod.id+'/'+value.slice('textures/'.length);
     for(const[key,value]of Object.entries(mod.manifest.meshes||{}))if(selected('mesh:'+key,mod.id))tables.meshes[key]='meshes/provider-'+mod.id+'/'+value.slice('meshes/'.length);
     for(const[key,value]of Object.entries(mod.manifest.materials))if(selected('material:'+key,mod.id))tables.materials[key]=value;
   }
   if(legacyIndex.length)await fs.writeFile(path.join(target,'textures/index.txt'),legacyIndex.join('\n')+'\n');
   // The legacy loader uses asset_000.bin to recognize a raw override directory.
   if(legacyIndex.length||active.some(m=>m.files.some(f=>f.startsWith('raw/')))){
     await fs.mkdir(path.join(target,'raw'),{recursive:true});try{await fs.access(path.join(target,'raw','asset_000.bin'));}catch{await fs.copyFile(path.join(this.state.disc.data,'raw','asset_000.bin'),path.join(target,'raw','asset_000.bin'));}
   }
   let manifest='[mod]\nid = "launcher-profile"\n';for(const group of ['textures','materials','meshes']){manifest+=`\n[${group}]\n`;for(const [key,value]of Object.entries(tables[group]))manifest+=`"${key}" = "${value}"\n`;}
   await fs.writeFile(path.join(target,'mod.toml'),manifest);
   const {run}=require('./service.cjs');await run(this.tool('rage-mod-cli'),[path.join(target,'mod.toml')]);
   return target;
   }catch(e){await fs.rm(target,{recursive:true,force:true});throw e;}
 };
}
module.exports={inventory,installMethods};
