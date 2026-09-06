const fs=require('node:fs/promises'),path=require('node:path');
module.exports=async function textures(service,key,geometry,metadata,staging,signal,format='rgba'){
 const {run}=require('./service.cjs');const [,kind,bankText]=key.split('.'),bank=Number(bankText),data=service.state.disc.data;
 const folder=path.join(data,'textures'),uploads=[];
 const entries=(await fs.readdir(folder)).filter(n=>n.endsWith('.json')).sort();
 // Shared images are loaded first. The selected car's atlas then overrides them.
 for(const own of [false,true]){
  // Boot image chains contain palette-only records absent from PNG sidecars.
  if(!own)uploads.push(path.join(data,'raw','asset_005.bin'),'-4','0','0','1','1');
  if(own&&kind==='player')uploads.push(path.join(data,'raw',`asset_${String(bank).padStart(3,'0')}.bin`),'-1','704','0','64','256');
  for(const name of entries){const match=name.match(/^asset_(\d+)_/);if(!match)continue;const asset=Number(match[1]);if(own?asset!==bank&&asset!==(kind==='player'?bank+1:bank-1):asset>9)continue;
   const m=JSON.parse(await fs.readFile(path.join(folder,name),'utf8'));const raw=path.join(data,'raw',`asset_${String(asset).padStart(3,'0')}.bin`);
   const b=m.vram;if(b&&Number.isInteger(m.pixels_offset))uploads.push(raw,String(m.pixels_offset),String(b.x),String(b.y),String(b.words),String(b.rows));
   if(m.clut){const c=m.clut;const w=Math.min(c.colours,1024-c.x);if(c.colours%w===0)uploads.push(raw,String(c.offset),String(c.x),String(c.y),String(w),String(c.colours/w));}
  }
 }
 if(kind==='rival')uploads.push(path.join(data,'raw',`asset_${String(bank-1).padStart(3,'0')}.bin`),'-3','0','0','1','1');
 const used=new Set();for(let i=6;i<geometry.vertices.length;i+=9)used.add((geometry.vertices[i]&0xffff)%64);
 const result={};
 for(const material of metadata.materials){if(!used.has(material.source))continue;const file=path.join(staging,'texture-'+material.source+'.'+format);await run(service.tool('rage-mod-cli'),['--texture',String(material.page),String(material.clut),file,...uploads],{signal});const bytes=await fs.readFile(file);if(format!=='png'&&bytes.length!==256*256*4+(format==='tga'?18:0))throw Error('Incomplete texture preview');result[material.source]=format!=='rgba'?path.basename(file):bytes.toString('base64');}
 return result;
};
