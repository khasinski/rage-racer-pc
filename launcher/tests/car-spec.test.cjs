const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {run}=require('../main/service.cjs');
const tool=path.resolve(__dirname,'../resources/bin/rage-mod-cli'+(process.platform==='win32'?'.exe':''));
test('car parameter edits preserve the source and unrelated pack bytes',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'rage-spec-'));
 try {
  const source=path.join(dir,'source.bin'),output=path.join(dir,'output.bin');
  const bytes=Buffer.alloc(2048,0xa5),base=32;
  [base,512,640,768,1024].forEach((v,i)=>bytes.writeUInt32LE(v,i*4));
  [8000,1000,5,7000].forEach((v,i)=>bytes.writeInt16LE(v,base+0x100+i*2));
  bytes.writeUInt16LE(40000,base+0x10a);
  await fs.writeFile(source,bytes);
  const read=JSON.parse(await run(tool,['--car-spec','read',source]));
  assert.equal(read.fields.find(f=>f.key==='revLimit').value,8000);
  assert.equal(read.fields.find(f=>f.key==='steerResponse').value,40000);
  await run(tool,['--car-spec','write',source,output,'revLimit','9000','topGear','6','downshift2','123','upshift2','456','steeringGripResponse','1200','steerResponse','32767','referenceTurnRadius','750','negconSteeringAssistScale','0','speedDragDivisor','2500','baseSteeringGrip','325','torqueCurve0','2147483647','torqueBand15','10000','gearRatio6','493','torqueScale5','-32768']);
  const expected=Buffer.from(bytes);expected.writeInt16LE(9000,base+0x100);expected.writeInt16LE(6,base+0x104);expected.writeInt16LE(123,base+0x124);expected.writeInt16LE(456,base+0x126);
  [1200,32767,750,0,2500,325].forEach((v,i)=>expected.writeInt16LE(v,base+0x108+i*2));
  expected.writeInt32LE(2147483647,base);expected.writeInt32LE(10000,base+0x7c);expected.writeInt32LE(493,base+0xfc);expected.writeInt16LE(-32768,base+0x11e);
  assert.deepEqual(await fs.readFile(output),expected);
  assert.deepEqual(await fs.readFile(source),bytes);
  await assert.rejects(run(tool,['--car-spec','write',source,source,'revLimit','9500']));
  await assert.rejects(run(tool,['--car-spec','write',source,output,'revLimit','9500']));
  assert.deepEqual(await fs.readFile(output),expected);
  for(const [key,value] of [['topGear','7'],['revLimit','0'],['redline','32768'],['unknown','1'],['revLimit','8000rpm']]){
   const invalid=path.join(dir,'invalid.bin');
   await assert.rejects(run(tool,['--car-spec','write',source,invalid,key,value]));
   await assert.rejects(fs.access(invalid));
  }
  const corrupt=Buffer.from(bytes);corrupt.writeUInt32LE(0xffffffff,0);
  await fs.writeFile(path.join(dir,'corrupt.bin'),corrupt);
  await assert.rejects(run(tool,['--car-spec','read',path.join(dir,'corrupt.bin')]));
  assert.deepEqual(await fs.readFile(source),bytes);
 } finally {await fs.rm(dir,{recursive:true,force:true});}
});

test('car parameters edit the shared project and apply without touching source packs',async()=>{
 const {LauncherService}=require('../main/service.cjs');
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-spec-service-'));
 try {
  const data=path.join(root,'data');await fs.mkdir(path.join(data,'raw'),{recursive:true});
  const bytes=Buffer.alloc(2048,0xa5);[32,512,640,768,1024].forEach((v,i)=>bytes.writeUInt32LE(v,i*4));
  [8000,1000,5,7000].forEach((v,i)=>bytes.writeInt16LE(v,32+0x100+i*2));
  const source=path.join(data,'raw/asset_011.bin');await fs.writeFile(source,bytes);
  const model=Buffer.alloc(128);model.writeUInt32LE(40,32);model.writeUInt32LE(80,36);model[8]=1;await fs.writeFile(path.join(data,'raw/asset_010.bin'),model);
  const archive=path.join(root,'RAGE.BIN');await fs.writeFile(archive,'test source');
  const service=new LauncherService({root:path.join(root,'profile'),modsRoot:path.join(root,'game-mods'),bin:path.dirname(tool),config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();service.state.disc={region:'NTSC-U',data,archive,path:archive};
  const original=await service.carSpec(10);
  assert.equal(original.fields.find(f=>f.key==='revLimit').origin,'built-in profile');
  const p=await service.saveCarSpec(10,{revLimit:9000,manualOnly:1,upshift3:1200,price:0,unlockClass:0,steeringGripResponse:1200,steerResponse:90,referenceTurnRadius:750,negconSteeringAssistScale:0,speedDragDivisor:2500,baseSteeringGrip:325,torqueCurve0:50000,gearRatio6:500,torqueScale5:99},original.revision);
  assert.equal(service.state.mods.length,0);assert.equal(p.dirty,true);
  const changed=await service.carSpec(10);
  assert.equal(changed.fields.find(f=>f.key==='revLimit').value,9000);
  assert.equal(changed.fields.find(f=>f.key==='revLimit').origin,'project');
  await service.applyProject(p.revision);
  const profile=await require('../main/project-build.cjs').activeProfile(service);
  const compiled=JSON.parse(await run(tool,['--car-profile',profile])).cars[0];
  assert.equal(compiled.rev_limit,9000);assert.equal(compiled.manual_only,true);assert.equal(compiled.shift_points[5],1200);assert.equal(compiled.price,0);
  assert.equal(compiled.torque_curve[0],50000);assert.equal(compiled.gear_ratio[6],500);assert.equal(compiled.torque_scale[5],99);
  assert.deepEqual(compiled.torque_curve.slice(1),original.fields.filter(f=>/^torqueCurve\d+$/.test(f.key)).slice(1).map(f=>f.value));
  for(const [field,value] of Object.entries({steering_grip_response:1200,steer_response:90,reference_turn_radius:750,negcon_steering_assist_scale:0,speed_drag_divisor:2500,base_steering_grip:325}))assert.equal(compiled[field],value);
  assert.deepEqual(await fs.readFile(source),bytes);assert.deepEqual(await fs.readFile(path.join(data,'raw/asset_010.bin')),model);
  for(const edits of [{torqueCurve16:1},{torqueCurve0:2147483648},{torqueScale0:32768},{speedDragDivisor:0},{steerResponse:32768},{negconSteeringAssistScale:-1},{revLimit:0},{topGear:7},{unknown:1},{revLimit:'9000'},{}])await assert.rejects(service.saveCarSpec(10,edits,p.revision));
  await assert.rejects(service.saveCarSpec(10,{revLimit:9500},original.revision),/changed/);
  await assert.rejects(service.carSpec(11));await assert.rejects(service.saveCarSpec(88,{revLimit:9000},p.revision));
  const undone=await service.projectHistory('undo',p.revision);assert.equal((await service.carSpec(10)).fields.find(f=>f.key==='torqueCurve0').value,original.fields.find(f=>f.key==='torqueCurve0').value);
  const redone=await service.projectHistory('redo',undone.revision);assert.equal((await service.carSpec(10)).fields.find(f=>f.key==='torqueCurve0').value,50000);
  const reset=await service.saveCarSpec(10,{torqueCurve0:original.fields.find(f=>f.key==='torqueCurve0').value},redone.revision);assert.ok(!reset.changes.some(c=>c.field==='torque_curve'));assert.ok(reset.changes.some(c=>c.field==='gear_ratio'));
  service.game={};await assert.rejects(service.applyProject(p.revision),/Close the game/);service.game=null;
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('player model layout preserves signed suspension offsets and mirrors the opposite front wheel',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-car-layout-'));
 try{
  const file=path.join(root,'model.bin'),bytes=Buffer.alloc(128);
  bytes.writeInt16LE(123,0);bytes.writeInt16LE(-45,2);bytes.writeInt16LE(-678,4);
  bytes.writeUInt32LE(40,32);bytes.writeUInt32LE(80,36);await fs.writeFile(file,bytes);
  const layout=JSON.parse(await run(tool,['--car-layout',file]));
  assert.deepEqual(layout.instances,[
   {part:0,position:[0,0,0],scale:[1,1,1]},
   {part:3,position:[0,0,0],scale:[1,1,1]},
   {part:2,position:[492,180,2712],scale:[1,1,1]},
   {part:2,position:[-492,180,2712],scale:[-1,1,-1]}
  ]);
  bytes.writeUInt32LE(0xffffffff,36);await fs.writeFile(file,bytes);
  await assert.rejects(run(tool,['--car-layout',file]));
  await fs.writeFile(file,bytes.subarray(0,10));await assert.rejects(run(tool,['--car-layout',file]));
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('rival layout selects the palette-zero suspension row for each body family',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-rival-layout-'));
 try{
  const file=path.join(root,'track.bin'),bytes=Buffer.alloc(1024);
  for(let i=0;i<11;i++)bytes.writeUInt32LE(i===0?44:200+(i-1)*64,i*4);
  for(let row=0;row<11;row++){bytes.writeInt16LE(10+row,44+12+row*8);bytes.writeInt16LE(-row,44+14+row*8);bytes.writeInt16LE(-70,44+16+row*8);}
  await fs.writeFile(file,bytes);
  for(const [body,row]of [[0,0],[20,4],[25,6],[30,8]]){
   const layout=JSON.parse(await run(tool,['--rival-layout',file,String(body)]));
   assert.deepEqual(layout.instances.map(i=>i.part),[body,body+3,body+2,body+2]);
   assert.deepEqual(layout.instances[2].position,[(10+row)*4,row*4,280]);
  }
  await assert.rejects(run(tool,['--rival-layout',file,'35']));
  bytes.writeUInt32LE(45,4);await fs.writeFile(file,bytes);
  await assert.rejects(run(tool,['--rival-layout',file,'0']));
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('MT-only writes the model flag without modifying geometry or the source',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'rage-transmission-'));
 try{
  const source=path.join(dir,'source.bin'),target=path.join(dir,'out.bin'),bytes=Buffer.alloc(256,0xa5);bytes.writeUInt32LE(40,32);bytes.writeUInt32LE(120,36);bytes[8]=1;await fs.writeFile(source,bytes);
  assert.equal(JSON.parse(await run(tool,['--car-transmission','read',source])).manualOnly,0);
  await run(tool,['--car-transmission','write',source,target,'1']);const expected=Buffer.from(bytes);expected[8]=0;assert.deepEqual(await fs.readFile(target),expected);assert.deepEqual(await fs.readFile(source),bytes);
  assert.equal(JSON.parse(await run(tool,['--car-transmission','read',target])).manualOnly,1);
  await assert.rejects(run(tool,['--car-transmission','write',source,source,'1']));
  await assert.rejects(run(tool,['--car-transmission','write',source,path.join(dir,'bad.bin'),'2']));
 }finally{await fs.rm(dir,{recursive:true,force:true});}
});
