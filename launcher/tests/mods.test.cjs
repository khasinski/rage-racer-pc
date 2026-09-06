const {test}=require('node:test');const assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {inventory}=require('../main/mods.cjs');
const {run}=require('../main/service.cjs');
test('maximum mesh manifest IPC fits its explicit response budget',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'rage-manifest-ipc-'));
 try{
  const file=path.join(dir,'mod.toml');
  const tool=path.resolve(__dirname,'../resources/bin/rage-mod-cli'+(process.platform==='win32'?'.exe':''));
  let text='[meshes]\n';
  for(let i=0;i<2048;i++)text+=`"car.${'a'.repeat(140)}.${i}"="meshes/${'b'.repeat(480)}.rmesh"\n`;
  assert.ok(Buffer.byteLength(text)<2*1024*1024);
  await fs.writeFile(file,text);
  const response=await run(tool,[file],{maxOutput:8*1024*1024});
  assert.ok(Buffer.byteLength(response)>2*1024*1024,'derived claims and paths exceed the generic IPC budget');
  const manifest=JSON.parse(response);
  assert.equal(Object.keys(manifest.meshes).length,2048);
  assert.equal(manifest.backingFiles.length,2048);
  assert.equal(manifest.resourceClaims.length,2048);
 }finally{await fs.rm(dir,{recursive:true,force:true});}
});
test('native preview decodes indexed pixels using the requested palette',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'rage-texture-test-'));
 try{
  const raw=path.join(dir,'raw.bin'),out=path.join(dir,'texture.rgba');
  const bytes=Buffer.alloc(34);bytes.writeUInt16LE(0x1111,0);bytes.writeUInt16LE(0x001f,4);
  await fs.writeFile(raw,bytes);
  const tool=path.resolve(__dirname,'../resources/bin/rage-mod-cli'+(process.platform==='win32'?'.exe':''));
  await run(tool,['--texture','0','64',out,raw,'0','0','0','1','1',raw,'2','0','1','16','1']);
  const rgba=await fs.readFile(out);assert.equal(rgba.length,256*256*4);
  assert.deepEqual([...rgba.subarray(0,4)],[255,0,0,255]);
  assert.deepEqual([...rgba.subarray(16,20)],[0,0,0,0]);
  const tga=path.join(dir,'texture.tga');
  await run(tool,['--texture','0','64',tga,raw,'0','0','0','1','1',raw,'2','0','1','16','1']);
  const image=await fs.readFile(tga);assert.equal(image.length,rgba.length+18);
  assert.equal(image.readUInt16LE(12),256);assert.equal(image.readUInt16LE(14),256);
  assert.equal(image[17],0x28,'top origin and eight alpha bits');
  assert.deepEqual([...image.subarray(18,22)],[0,0,255,255],'TGA stores BGRA');
  await assert.rejects(run(tool,['--texture','0','64',out,raw,'32','0','0','2','1']));
 }finally{await fs.rm(dir,{recursive:true,force:true});}
});
test('the actual runtime manifest parser rejects invalid materials and unsafe paths',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'rage-manifest-test-'));
 const file=path.join(dir,'mod.toml'),tool=path.resolve(__dirname,'../resources/bin/rage-mod-cli'+(process.platform==='win32'?'.exe':''));
 try{
  await fs.writeFile(file,'[mod]\nid = "example"\n[textures]\n"car.0.material.1" = "textures/body.png"\n');
  assert.equal(JSON.parse(await run(tool,[file])).textures['car.0.material.1'],'textures/body.png');
  await fs.writeFile(file,'[textures]\n"car.0.material.1" = "textures/body.png"\n[meshes]\n"car.player.0.part.0" = "meshes/body.rmesh"\n');
  assert.deepEqual(JSON.parse(await run(tool,[file])).backingFiles,['textures/body.png','meshes/body.rmesh']);
  assert.deepEqual(JSON.parse(await run(tool,[file])).resourceClaims,['texture:car.0.material.1','mesh:car.player.0.part.0']);
  for(const text of ['[textures]\n"car.0.material.1" = "textures/../../outside.png"','[materials]\n"car.0.material.1" = "lit opaque 2 0 1 1 1 1 0 0 0"']){
   await fs.writeFile(file,text);await assert.rejects(run(tool,[file]));
  }
 }finally{await fs.rm(dir,{recursive:true,force:true});}
});
test('mod inventory excludes arbitrary files and symlinks',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'rage-mod-test-'));try{
 await fs.mkdir(path.join(dir,'raw'));await fs.writeFile(path.join(dir,'raw','asset_010.bin'),'asset');
 assert.deepEqual(await inventory(dir),['raw/asset_010.bin']);
 await fs.writeFile(path.join(dir,'run.js'),'bad');await assert.rejects(inventory(dir));await fs.rm(path.join(dir,'run.js'));
 if(process.platform!=='win32'){await fs.symlink(path.join(dir,'raw','asset_010.bin'),path.join(dir,'raw','asset_011.bin'));await assert.rejects(inventory(dir));}
 }finally{await fs.rm(dir,{recursive:true,force:true});}
});

test('raw overrides are limited to the 135 archive entries',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-raw-range-'));
 try{
  await fs.mkdir(path.join(root,'raw'));
  await fs.writeFile(path.join(root,'raw/asset_134.bin'),'last entry');
  assert.deepEqual(await inventory(root),['raw/asset_134.bin']);
  for(const index of ['135','999']){
   const file=path.join(root,`raw/asset_${index}.bin`);await fs.writeFile(file,'invalid');
   await assert.rejects(inventory(root),/Unsupported mod file/);await fs.rm(file);
  }
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('boot image chains restore palette-only records used by rival wheels',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-boot-palette-'));
 try{
  const chain=Buffer.alloc(76);chain.writeUInt32LE(64,4);chain.writeUInt32LE(16,8);chain.writeUInt32LE(8,12);
  chain.writeUInt32LE(44,16);chain.writeUInt16LE(96,20);chain.writeUInt16LE(480,22);chain.writeUInt16LE(16,24);chain.writeUInt16LE(1,26);chain.writeUInt16LE(31,30);
  chain.writeUInt32LE(12,60); // Empty pixel block: the entry only uploads a palette.
  const source=path.join(root,'boot.bin'),pixels=path.join(root,'pixels.bin'),out=path.join(root,'texture.rgba');
  await fs.writeFile(source,chain);await fs.writeFile(pixels,Buffer.from([0x11,0x11]));
  const tool=path.resolve(__dirname,'../resources/bin/rage-mod-cli'+(process.platform==='win32'?'.exe':''));
  await run(tool,['--texture','10','30726',out,source,'-4','0','0','1','1',pixels,'0','640','0','1','1']);
  assert.deepEqual([...(await fs.readFile(out)).subarray(0,4)],[255,0,0,255]);
  await fs.writeFile(source,chain.subarray(0,71));
  await assert.rejects(run(tool,['--texture','10','30726',out,source,'-4','0','0','1','1']));
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
