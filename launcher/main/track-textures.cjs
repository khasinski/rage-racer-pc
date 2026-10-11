const fs=require('node:fs/promises'),path=require('node:path');
module.exports=async function trackTextures(service,bank,banks,metadata,directory,files,signal){
 const {run}=require('./service.cjs'),requests=[],unique=new Map(),bindings=[];
 const track=['big','mid','hi','oval'][((bank-88)/2)%4]+(Math.floor((bank-88)/8)+1);
 for(const [name,{geometry,instances}]of Object.entries(banks)){
  const used=new Set();
  for(const instance of instances){const [first,count]=geometry.ranges[instance.mesh];for(let i=first;i<first+count;i++)used.add(geometry.vertices[geometry.indices[i]*9+6]&65535);}
  geometry.textures={};geometry.materials={};
  for(const m of metadata[name].materials){
   if(!used.has(m.source))continue;
   const descriptor=[m.page,m.clut,m.windowWidthU,m.windowWidthV,m.windowOffsetU,m.windowOffsetV],key=descriptor.join(':');
   if(!unique.has(key)){unique.set(key,requests.length);requests.push(descriptor);}
   bindings.push({name,source:m.source,request:unique.get(key),id:`track.${track}.${name}.material.${m.source}`});
  }
 }
 if(!requests.length)return;
 const raw=asset=>{const relative=`raw/asset_${String(asset).padStart(3,'0')}.bin`;return path.join(files.includes(relative)?directory:service.state.disc.data,relative);};
 const uploads=[[raw(5),-4,0,0,1,1],[raw(bank-1),-3,0,0,1,1]];
 const folder=path.join(directory,'preview-textures');await fs.mkdir(folder);
 await run(service.tool('rage-mod-cli'),['--texture-batch-stdin',folder],{input:Buffer.from(JSON.stringify({uploads,textures:requests})),signal});
 const images=[];for(let i=0;i<requests.length;i++)images.push({png:(await fs.readFile(path.join(folder,`texture-${i}.png`))).toString('base64')});
 let manifest={textures:{},materials:{}};
 if(files.includes('mod.toml'))manifest=JSON.parse(await run(service.tool('rage-mod-cli'),[path.join(directory,'mod.toml')],{signal}));
 const properties={...manifest.materials,...Object.fromEntries((service.editor?.project.snapshot().changes||[]).filter(c=>c.field==='material').map(c=>[c.id,c.value]))};
 for(const b of bindings){
  const relative=manifest.textures[b.id+'.variant.0']??manifest.textures[b.id];
  const texture=relative?{png:(await fs.readFile(path.join(directory,relative))).toString('base64')}:images[b.request];
  banks[b.name].geometry.textures[b.source]=texture;
  banks[b.name].geometry.materials[b.source]={id:b.id,properties:properties[b.id+'.variant.0']??properties[b.id]};
 }
};
