const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {run}=require('../main/service.cjs');
test('record editing preserves other save fields and the original file',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-save-records-'));
 const suffix=process.platform==='win32'?'.exe':'';
 const build=process.env.RAGE_LAUNCHER_BUILD_DIR||path.resolve(__dirname,'../../build/release');
 let generator=path.join(build,'rage-save-generator'+suffix);
 try{await fs.access(generator);}catch{generator=path.join(build,'Release','rage-save-generator'+suffix);}
 const tool=path.resolve(__dirname,'../resources/bin/rage-save-cli'+suffix);
 try{
  const original=path.join(root,'original.save'),copy=path.join(root,'copy.save');
  await run(generator,['--output',original,'--name','TEST']);
  const bytes=await fs.readFile(original),before=JSON.parse(await run(tool,['read',original]));
  const changes={
   'Ranking / Set 0 / Course 0 / Slot 0 / Driver':'RIVAL',
   'Ranking / Set 0 / Course 0 / Slot 0 / Car':'10',
   'Grand Prix / Car 03 / Transmission':'1',
   'Best lap / Set 0 / Course 0 / Slot 0':'-1',
  };
  const pixels=before.logo.pixels.split('');
  for(const [index,color]of [[0,'1'],[63,'2'],[4032,'e'],[4095,'f']])pixels[index]=color;
  changes['logo.pixels']=pixels.join('');
  changes['logo.color.1']='ff00001';
  const after=JSON.parse(await run(tool,['write',original,copy,...Object.entries(changes).flat()]));
  assert.equal(after.checksumsValid,true);
  assert.deepEqual(await fs.readFile(original),bytes);
  assert.equal(after.carNames.international[10],'Vainqure');
  assert.equal(after.carNames.japanese[10],'Victoire');
  assert.equal(after.logo.pixels,changes['logo.pixels']);
  assert.deepEqual(after.logo.colors[1],{rgb:'#ff0000',transparent:true});
  for(let i=0;i<16;i++)if(i!==1)assert.deepEqual(after.logo.colors[i],before.logo.colors[i]);
  for(const field of before.fields){
   const actual=after.fields.find(f=>f.key===field.key);
   if(Object.hasOwn(changes,field.key))assert.equal(String(actual.value),changes[field.key]);
   else assert.deepEqual(actual,field);
  }
  for(const value of ['TOO-LONG-NAME','BAD\nNAME']){
   const failed=path.join(root,'invalid.save');
   await assert.rejects(run(tool,['write',original,failed,'Ranking / Set 0 / Course 0 / Slot 0 / Driver',value]));
   await assert.rejects(fs.access(failed));
  }
  for(const [key,value]of [['logo.pixels','0'.repeat(4095)],['logo.pixels','g'.repeat(4096)],['logo.color.16','ff00000'],['logo.color.0','0000002']]){
   const failed=path.join(root,'invalid-logo.save');
   await assert.rejects(run(tool,['write',original,failed,key,value]));
   await assert.rejects(fs.access(failed));
  }
  // A synthetic card has unrelated, nonzero data around the selected save.
  // This independently checks that card editing preserves that data, including
  // the DexDrive wrapper and unused bytes in the selected 8K block.
  for(const headerSize of [0,3904]){
   const image=Buffer.alloc(headerSize+128*1024,0x7b);
   if(headerSize)image.write('123-456-STD',0,'ascii');
   image.write('MC',headerSize,'ascii');
   const entry=headerSize+3*128;image.fill(0,entry,entry+128);
   image[entry]=0x51;image.writeUInt32LE(8192,entry+4);image.writeUInt16LE(0xffff,entry+8);
   image.write('BASLUS-00403 RAGE001',entry+10,'ascii');
   for(let i=0;i<127;i++)image[entry+127]^=image[entry+i];
   const start=headerSize+3*8192;bytes.copy(image,start);
   const input=path.join(root,headerSize?'card.gme':'card.mcr');
   const output=path.join(root,headerSize?'card-copy.gme':'card-copy.mcr');
   await fs.writeFile(input,image);
   const list=JSON.parse(await run(tool,['read',input]));
   assert.equal(list.card,true);assert.equal(list.entries.length,1);
   await run(tool,['write',input,output,'--card','0','logo.pixels',changes['logo.pixels'],'Ranking / Set 0 / Course 0 / Slot 0 / Driver','CARD']);
   const edited=JSON.parse(await run(tool,['read',output,'--card','0']));
   assert.equal(edited.checksumsValid,true);assert.equal(edited.logo.pixels,changes['logo.pixels']);
   assert.equal(edited.fields.find(f=>f.key==='Ranking / Set 0 / Course 0 / Slot 0 / Driver').value,'CARD');
   const result=await fs.readFile(output);
   assert.deepEqual(result.subarray(0,start),image.subarray(0,start));
   assert.deepEqual(result.subarray(start+bytes.length),image.subarray(start+bytes.length));
   assert.deepEqual(await fs.readFile(input),image);
  }
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('save-copy service holds the operation lock and returns the committed native result',async()=>{
 const {LauncherService}=require('../main/service.cjs');
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-save-lock-'));
 const suffix=process.platform==='win32'?'.exe':'';
 const build=process.env.RAGE_LAUNCHER_BUILD_DIR||path.resolve(__dirname,'../../build/release');
 let generator=path.join(build,'rage-save-generator'+suffix);try{await fs.access(generator);}catch{generator=path.join(build,'Release','rage-save-generator'+suffix);}
 try{
  const original=path.join(root,'original.save'),copy=path.join(root,'copy.save');await run(generator,['--output',original,'--name','LOCK']);const before=await fs.readFile(original);
  const service=new LauncherService({root,bin:path.resolve(__dirname,'../resources/bin'),config:path.resolve(__dirname,'../resources/rage-port.ini')});await service.init();service.state.disc={region:'NTSC-U'};
  const pending=service.writeSave(original,copy,undefined,{'Grand Prix / Car 00 / Paint 1':'9'});
  try{
   assert.equal(service.busy.cancellable,false);
   await assert.rejects(service.launch(),/current operation/);
   await assert.rejects(service.writeSave(original,path.join(root,'other.save'),undefined,{}),/current operation/);
   const result=await pending;assert.equal(result.checksumsValid,true);assert.equal(service.busy,null);
   const reread=await service.readSave(copy);assert.deepEqual(result,reread);assert.deepEqual(await fs.readFile(original),before);
   await assert.rejects(fs.access(path.join(root,'other.save')));
  }finally{await pending;}
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('save-copy rejects aliases of the original before invoking native editing',async()=>{
 const {LauncherService}=require('../main/service.cjs');
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-save-alias-'));
 try{
  const sourceDir=path.join(root,'source'),aliasDir=path.join(root,'alias');await fs.mkdir(sourceDir);
  const original=path.join(sourceDir,'original.save');await fs.writeFile(original,'source must survive');
  // Windows junctions do not require developer-mode symlink privileges.
  await fs.symlink(sourceDir,aliasDir,process.platform==='win32'?'junction':'dir');
  const service=new LauncherService({root,bin:path.join(root,'no-tools')});service.state.disc={region:'NTSC-U'};
  await assert.rejects(service.writeSave(original,path.join(aliasDir,'original.save'),undefined,{}),/preserve the original/);
  const hardlink=path.join(root,'hardlink.save');await fs.link(original,hardlink);
  await assert.rejects(service.writeSave(original,hardlink,undefined,{}),/preserve the original/);
  assert.equal(await fs.readFile(original,'utf8'),'source must survive');assert.equal(service.busy,null);
  assert.deepEqual(await fs.readdir(sourceDir),['original.save']);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('integrated editor creates and edits all regions without a game image',async()=>{
 const {LauncherService}=require('../main/service.cjs');
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-save-integrated-'));
 try{
  const service=new LauncherService({root,bin:path.resolve(__dirname,'../resources/bin')});
  assert.equal(service.state.disc,null);
  for(const [region,prefix]of [['PAL','BESCES-00650'],['NTSC-U','BASLUS-00403'],['NTSC-J','BISLPS-00600']]){
   const file=path.join(root,prefix+' RAGE000'),copy=path.join(root,prefix+' RAGE001');
   const created=await service.newSave(file,region);
   assert.equal(created.region,region);assert.equal(created.checksumsValid,true);
   const original=await fs.readFile(file),opened=await service.readSave(file);
   assert.equal(opened.region,region);
   const values=Object.fromEntries(opened.fields.map(f=>[f.key,f.value]));
   assert.equal(values['Grand Prix / Car 03 / Owned'],1);
   assert.equal(values['Grand Prix / Highest class'],-1);
   const edits={'Advanced / Save counter':'42','Advanced / Reserved byte 00':'173',team:'MERGED'};
   const changed=await service.writeSave(file,copy,undefined,edits);
   assert.equal(changed.checksumsValid,true);assert.equal(changed.team,'MERGED');
   for(const key of Object.keys(edits).filter(k=>k!=='team'))assert.equal(String(changed.fields.find(f=>f.key===key).value),edits[key]);
   assert.deepEqual(await fs.readFile(file),original);
   assert.equal((await service.readSave(copy)).checksumsValid,true);
  }
  await assert.rejects(service.newSave(path.join(root,'invalid'),'OTHER'),/Invalid save region/);
  service.game={};await assert.rejects(service.newSave(path.join(root,'running'),'PAL'),/Close the game/);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
