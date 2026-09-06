const path=require('node:path');
const fs=require('node:fs/promises');
module.exports=async function(service,key,metadata,mod,signal){
 const properties=mod?.manifest.materials||{},textures=mod?.manifest.textures||{};if(!Object.keys(properties).length&&!Object.keys(textures).length)return {};
 const [,kind,bankText]=/^car\.(player|rival)\.(\d+)\.part\.\d+$/.exec(key),bank=Number(bankText);
 const index=(bank-88)/2;
 const prefix=kind==='player'?`car.${(bank-10)/2}.material.`:`track.${['big','mid','hi','oval'][index%4]}${Math.floor(index/4)+1}.model-bank-1.material.`;
 if(![...Object.keys(properties),...Object.keys(textures)].some(id=>id.startsWith(prefix)))return {};
 const {run}=require('./service.cjs');
 const file=path.join(service.state.disc.data,'raw',`asset_${bankText.padStart(3,'0')}.bin`);
 const table=JSON.parse(await run(service.tool('rage-mod-cli'),['--car-materials',kind,file,bankText],{signal}));
 const result={};
 for(const material of metadata.materials){
  const slot=table.materials.find(m=>m.page===material.page&&m.clut===material.clut);if(!slot)continue;
  const value=properties[slot.id+'.variant.0']??properties[slot.id];
  const relative=textures[slot.id+'.variant.0']??textures[slot.id];
  if(value!==undefined||relative!==undefined)result[material.source]={id:slot.id,...(value===undefined?{}:{properties:value})};
  if(relative!==undefined){
   if(!mod.files.includes(relative)||!/^textures\/[A-Za-z0-9_./-]+\.png$/.test(relative)||relative.split('/').some(p=>!p||p==='.'||p==='..'))throw Error('Invalid preview texture path');
   const file=path.join(service.root,'mods',mod.id,relative);if((await fs.stat(file)).size>32*1024*1024)throw Error('Preview texture exceeds 32 MiB');
   const bytes=await fs.readFile(file);if(bytes.length<24||!bytes.subarray(0,8).equals(Buffer.from([137,80,78,71,13,10,26,10])))throw Error('Invalid preview PNG');
   const width=bytes.readUInt32BE(16),height=bytes.readUInt32BE(20);if(!width||!height||width>4096||height>4096)throw Error('Preview textures support dimensions up to 4096 × 4096');
   result[material.source].texture={png:bytes.toString('base64')};
  }
 }
 return result;
};
