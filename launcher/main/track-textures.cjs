const fs=require('node:fs/promises'),path=require('node:path');
function nearRef(v,ref){return ref+(((((Number(v)||0)-ref)<<16)>>16));}
function scanTexBlock(buf,b){
 const cluts=new Set(),pairs=new Set();
 if(!buf||buf.length<20)return {cluts,pairs};
 let at=buf.readUInt32LE(b*4)+4,curClut=null;
 while(at+4<=buf.length){
  const n=buf.readUInt32LE(at);at+=4;if(!n||n>0x7fffffff||at+n>buf.length)break;
  const count=(buf.readUInt32LE(at+4)&8)?2:1;let cur=at+8;
  for(let k=0;k<count&&cur+12<=at+n;k++){
   const len=buf.readUInt32LE(cur),bx=buf.readUInt16LE(cur+4),by=buf.readUInt16LE(cur+6),w=buf.readUInt16LE(cur+8),h=buf.readUInt16LE(cur+10);
   if(w>0&&h>0){if(bx<576){curClut=`${bx},${by}`;cluts.add(curClut);}else if(curClut)pairs.add(`${bx>>6}:${curClut}`);}
   cur+=len;
  }
  at+=n;
 }
 return {cluts,pairs};
}
function assignSectionPages(trackBuf,banks){
 if(!trackBuf||trackBuf.length<44)return;
 const off0=trackBuf.readUInt32LE(0),off4=trackBuf.readUInt32LE(16);
 if(off0+8>trackBuf.length||off4+4>trackBuf.length)return;
 const lo=trackBuf.readInt32LE(off0),hi=trackBuf.readInt32LE(off0+4),ptCount=trackBuf.readUInt32LE(off4);
 if(!ptCount||off4+4+ptCount*24>trackBuf.length)return;
 const roadPts=[];let cum=0;
 for(let i=0;i<ptCount;i++){
  const p=off4+4+i*24,sec=cum>>8;
  roadPts.push({x:nearRef(trackBuf.readInt32LE(p),32768),z:-nearRef(trackBuf.readInt32LE(p+4),32768),page:sec>=lo&&sec<hi?1:0});
  cum+=trackBuf.readUInt16LE(p+22);
 }
 for(const {instances}of Object.values(banks))for(const inst of instances){
  if(!Array.isArray(inst.position)||(inst.cell===undefined&&inst.object===undefined))continue;
  let best=Infinity,page=0;
  for(const rp of roadPts){const d=(rp.x-inst.position[0])**2+(rp.z-inst.position[2])**2;if(d<best){best=d;page=rp.page;}}
  inst.page=page;
 }
}
module.exports=async function trackTextures(service,bank,banks,metadata,directory,files,signal,view){
 const sourceRoot=view?.root||directory,sourceFiles=view?.files||files;
 const {run}=require('./service.cjs'),requests=[],unique=new Map(),requestsP1=[],uniqueP1=new Map(),bindings=[];
 const track=['big','mid','hi','oval'][((bank-88)/2)%4]+(Math.floor((bank-88)/8)+1);
 const raw=asset=>{const relative=`raw/asset_${String(asset).padStart(3,'0')}.bin`;return path.join(sourceFiles.includes(relative)?sourceRoot:service.state.disc.data,relative);};
 const [texBuf,trackBuf]=await Promise.all([fs.readFile(raw(bank-1)).catch(()=>null),fs.readFile(raw(bank)).catch(()=>null)]);
 const b3=scanTexBlock(texBuf,3),b4=scanTexBlock(texBuf,4);
 assignSectionPages(trackBuf,banks);
 for(const [name,{geometry,instances}]of Object.entries(banks)){
  const used=new Set(),usedP1=new Set();
  for(const instance of instances){
   const [first,count]=geometry.ranges[instance.mesh];
   for(let i=first;i<first+count;i++){
    const src=geometry.vertices[geometry.indices[i]*9+6]&65535;
    used.add(src);if(instance.page)usedP1.add(src);
   }
  }
  geometry.textures={};geometry.texturesPage1={};geometry.materials={};
  for(const m of metadata[name].materials){
   if(!used.has(m.source))continue;
   const descriptor=[m.page,m.clut,m.windowWidthU,m.windowWidthV,m.windowOffsetU,m.windowOffsetV],key=descriptor.join(':');
   const tpX=m.page&15,py=((m.page>>4)&1)*256,swapped=tpX>=9&&py>=256;
   const cKey=`${(m.clut&63)*16},${(m.clut>>6)&511}`,tcKey=`${tpX}:${cKey}`;
   const onlyPage1=swapped&&((b3.pairs.has(tcKey)&&!b4.pairs.has(tcKey))||(b3.cluts.has(cKey)&&!b4.cluts.has(cKey)));
   const needPage1=onlyPage1||(swapped&&usedP1.has(m.source));
   if(!unique.has(key)){unique.set(key,requests.length);requests.push(descriptor);}
   let requestP1;
   if(needPage1){if(!uniqueP1.has(key)){uniqueP1.set(key,requestsP1.length);requestsP1.push(descriptor);}requestP1=uniqueP1.get(key);}
   bindings.push({name,source:m.source,request:unique.get(key),requestP1,onlyPage1,id:`track.${track}.${name}.material.${m.source}`});
  }
 }
 if(!requests.length)return;
 const uploads=[[raw(5),-4,0,0,1,1],[raw(bank-1),-3,0,0,1,1]];
 const folder=path.join(directory,'preview-textures');await fs.mkdir(folder);
 await run(service.tool('rage-mod-cli'),['--texture-batch-stdin',folder],{input:Buffer.from(JSON.stringify({uploads,textures:requests})),signal});
 const images=[];for(let i=0;i<requests.length;i++)images.push({png:(await fs.readFile(path.join(folder,`texture-${i}.png`))).toString('base64')});
 const imagesP1=[];
 if(requestsP1.length&&texBuf&&texBuf.length>=20){
  const s3=texBuf.readUInt32LE(12),e3=texBuf.readUInt32LE(16);
  if(s3<e3&&e3<=texBuf.length){
   const page1File=path.join(directory,'page1.bin'),folderP1=path.join(directory,'preview-textures-p1');
   await fs.writeFile(page1File,texBuf.subarray(s3,e3));await fs.mkdir(folderP1);
   await run(service.tool('rage-mod-cli'),['--texture-batch-stdin',folderP1],{input:Buffer.from(JSON.stringify({uploads:[...uploads,[page1File,-4,0,0,1,1]],textures:requestsP1})),signal});
   for(let i=0;i<requestsP1.length;i++)imagesP1.push({png:(await fs.readFile(path.join(folderP1,`texture-${i}.png`))).toString('base64')});
  }
 }
 let manifest={textures:{},materials:{}};
 if(sourceFiles.includes('mod.toml'))manifest=JSON.parse(await run(service.tool('rage-mod-cli'),[path.join(sourceRoot,'mod.toml')],{signal}));
 const projectProperties=Object.fromEntries((service.editor?.project.snapshot().changes||[]).filter(c=>c.field==='material').map(c=>[c.id,c.value]));
 for(const b of bindings){
  const relative=manifest.textures[b.id+'.variant.0']??manifest.textures[b.id];
  const baseImg=(b.onlyPage1&&imagesP1[b.requestP1])?imagesP1[b.requestP1]:images[b.request];
  const texture=relative?{png:(await fs.readFile(path.join(sourceRoot,relative))).toString('base64')}:baseImg;
  const change=service.editor?.project.snapshot().changes.find(c=>c.kind==='track'&&c.id===b.id&&c.field==='texture');
  const finalTex=change?{png:(await new (require('./project-files.cjs').ProjectFiles)(service.editor.file).read(change.value)).toString('base64')}:texture;
  banks[b.name].geometry.textures[b.source]=finalTex;
  if(b.requestP1!==undefined&&imagesP1[b.requestP1]){
   const relP1=manifest.textures[b.id+(b.name==='terrain'?'.variant.2':'.variant.4')]??relative;
   banks[b.name].geometry.texturesPage1[b.source]=change?finalTex:relP1?{png:(await fs.readFile(path.join(sourceRoot,relP1))).toString('base64')}:imagesP1[b.requestP1];
  }
  banks[b.name].geometry.materials[b.source]={id:b.id,properties:projectProperties[b.id+'.variant.0']??projectProperties[b.id]??manifest.materials[b.id+'.variant.0']??manifest.materials[b.id]};
 }
};
