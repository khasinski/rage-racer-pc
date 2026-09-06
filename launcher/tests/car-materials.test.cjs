const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');const {run}=require('../main/service.cjs');
test('native car material slots follow first textured occurrence and reject truncated streams',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-material-slots-'));const tool=path.resolve(__dirname,'../resources/bin/rage-mod-cli'+(process.platform==='win32'?'.exe':''));
 try{for(const kind of ['player','rival']){
  const offset=kind==='player'?40:44,pack=Buffer.alloc(220),bank=pack.subarray(offset,offset+128);
  if(kind==='player'){pack.writeUInt32LE(128,24);pack.writeUInt32LE(40,32);pack.writeUInt32LE(168,36);}else{pack.writeUInt32LE(44,12);pack.writeUInt32LE(172,16);}
  bank.writeUInt32LE(1,0);bank.writeUInt32LE(100,4);bank.writeUInt32LE(108,8);bank.writeUInt32LE(16,12);bank.writeUInt16LE(1,16);bank.writeUInt16LE(3,18);
  [[11,22],[9,33],[11,22]].forEach(([page,clut],i)=>{bank.writeUInt16LE(page,20+i*24+14);bank.writeUInt16LE(clut,20+i*24+10);});
  const file=path.join(root,kind+'.bin');await fs.writeFile(file,pack);
  const result=JSON.parse(await run(tool,['--car-materials',kind,file]));assert.deepEqual(result.materials,[{slot:0,page:11,clut:22},{slot:1,page:9,clut:33}]);
  const identified=JSON.parse(await run(tool,['--car-materials',kind,file,kind==='player'?'10':'88']));
  assert.deepEqual(identified.materials.map(m=>m.id),kind==='player'?['car.0.material.0','car.0.material.1']:['track.big1.model-bank-1.material.0','track.big1.model-bank-1.material.1']);
  await assert.rejects(run(tool,['--car-materials',kind,file,'1']));
  const preview=require('../main/preview-materials.cjs'),asset=kind==='player'?'010':'088';
  await fs.mkdir(path.join(root,'raw'),{recursive:true});await fs.copyFile(file,path.join(root,'raw','asset_'+asset+'.bin'));
  await fs.mkdir(path.join(root,'mods','sample','textures'),{recursive:true});
  const png=Buffer.alloc(24);Buffer.from([137,80,78,71,13,10,26,10]).copy(png);png.writeUInt32BE(1,16);png.writeUInt32BE(1,20);await fs.writeFile(path.join(root,'mods','sample','textures','variant.png'),png);
  const id=identified.materials[0].id,mod={id:'sample',files:['textures/variant.png'],manifest:{materials:{[id]:'lit opaque 0.5 0 1 1 1 1 0 0 0'},textures:{[id]:'textures/missing.png',[id+'.variant.0']:'textures/variant.png'}}};
  const service={root,state:{disc:{data:root}},tool:()=>tool};
  const mapped=await preview(service,'car.'+kind+'.'+Number(asset)+'.part.0',{materials:[{source:7,page:11,clut:22}]},mod);
  assert.equal(mapped[7].id,id);assert.equal(mapped[7].texture.png,png.toString('base64'));
  delete mod.manifest.textures[id+'.variant.0'];await assert.rejects(preview(service,'car.'+kind+'.'+Number(asset)+'.part.0',{materials:[{source:7,page:11,clut:22}]},mod),/Invalid preview texture path/);
  bank.writeUInt16LE(100,18);await fs.writeFile(file,pack);await assert.rejects(run(tool,['--car-materials',kind,file]),/Invalid car model material table/);
 }}finally{await fs.rm(root,{recursive:true,force:true});}
});
