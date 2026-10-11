const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {LauncherService}=require('../main/service.cjs');
test('complete car import installs all models and textures atomically and rejects wrong slots and escaped paths',async()=>{
 // Exercise native OBJ conversion, mesh inspection and PNG decoding through
 // paths outside a single Windows legacy code page, including spaces.
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-car-set-Łódź-日本-🚗 '));
 try{
  const source=path.join(root,'source'),data=path.join(root,'data');await fs.mkdir(source);await fs.mkdir(path.join(data,'raw'),{recursive:true});
  const raw=Buffer.alloc(220),bank=raw.subarray(40,168);raw.writeUInt32LE(128,24);raw.writeUInt32LE(40,32);raw.writeUInt32LE(168,36);bank.writeUInt32LE(1,0);bank.writeUInt32LE(100,4);bank.writeUInt32LE(108,8);bank.writeUInt32LE(16,12);bank.writeUInt16LE(1,16);bank.writeUInt16LE(1,18);bank.writeUInt16LE(11,34);bank.writeUInt16LE(22,30);await fs.writeFile(path.join(data,'raw/asset_010.bin'),raw);
  await fs.writeFile(path.join(source,'model.obj'),'v 0 0 0\nv 1 0 0\nv 0 1 0\nvt 0 0\nvn 0 0 1\nusemtl rage_0\nf 1/1/1 2/1/1 3/1/1\n');
  await fs.writeFile(path.join(source,'image.png'),Buffer.from('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVQIHWP4z8DwHwAFgAI/ScLttAAAAABJRU5ErkJggg==','base64'));
  const key='car.player.10.part.0',manifest={format:1,region:'NTSC-U',key,parts:{body:'model.obj','front-wheels':'model.obj','rear-wheels':'model.obj'},textures:{0:'image.png'}};
  const write=()=>fs.writeFile(path.join(source,'car-set.json'),JSON.stringify(manifest));await write();
  const service=new LauncherService({root:path.join(root,'profile'),bin:path.resolve(__dirname,'../resources/bin'),config:path.resolve(__dirname,'../resources/rage-port.ini')});await service.init();service.state.disc={region:'NTSC-U',data};service.carTool=async args=>JSON.stringify(args.some(a=>a.startsWith('tools.car_export='))?{materials:[{source:0,page:11,clut:22}]}:{parts:[{key,bank:10,part:0,rival:false,name:'Esperanza'}]});
  const id=await service.importCarSet(key,source),mod=service.state.mods.find(m=>m.id===id);assert.equal(mod.enabled,false);assert.deepEqual(Object.keys(mod.manifest.meshes),[key,'car.player.10.part.2','car.player.10.part.3']);assert.ok(mod.manifest.textures['car.0.material.0']);
  manifest.key='car.player.12.part.0';await write();await assert.rejects(service.importCarSet(key,source),/slot/);manifest.key=key;
  manifest.parts['rear-wheels']='../outside.obj';await write();await assert.rejects(service.importCarSet(key,source),/path/);assert.equal(service.state.mods.length,1);assert.equal((await fs.readdir(service.root)).some(n=>n.startsWith('car-set-import-')),false);
  manifest.parts['rear-wheels']='broken.obj';await fs.writeFile(path.join(source,'broken.obj'),'not a model');await write();await assert.rejects(service.importCarSet(key,source));assert.equal(service.state.mods.length,1);
  manifest.parts['rear-wheels']='model.obj';await write();
  const png=await fs.readFile(path.join(source,'image.png'));
  // The complete signature and dimensions look valid, but there are no pixels.
  await fs.writeFile(path.join(source,'image.png'),png.subarray(0,33));
  await assert.rejects(service.importCarSet(key,source),/Cannot import texture image\.png/);
  assert.equal(service.state.mods.length,1);
  assert.equal((await fs.readdir(service.root)).some(n=>n.startsWith('car-set-import-')),false);
  await fs.writeFile(path.join(source,'image.png'),png);
  const obj=await fs.readFile(path.join(source,'model.obj'),'utf8');
  await fs.writeFile(path.join(source,'model.obj'),obj.replace('rage_0','rage_63'));
  await assert.rejects(service.importCarSet(key,source),/body.*texture source 63/);
  assert.equal(service.state.mods.length,1);
  await assert.rejects(service.importCar(key,path.join(source,'model.obj')),/model\.obj.*texture source 63/);
  await fs.writeFile(path.join(source,'model.obj'),obj.replace('rage_0','rage_384'));
  await assert.rejects(service.importCarSet(key,source),/unsupported car surface/);
  // Explicit vertex-colour surfaces do not need a texture binding.
  await fs.writeFile(path.join(source,'model.obj'),obj.replace('rage_0','rage_65535'));
  const plainId=await service.importCar(key,path.join(source,'model.obj'));
  assert.ok(service.state.mods.some(m=>m.id===plainId));
  // An imported source owns edits directly, with one history action for
  // body, both wheel models and their shared texture.
  service.modsRoot=path.join(root,'game-mods');
  const archive=path.join(root,'RAGE.BIN');await fs.writeFile(archive,'car set source');service.state.disc.archive=archive;
  await fs.writeFile(path.join(source,'model.obj'),obj);await write();
  const before=await service.project(),count=service.state.mods.length;
  let project=await service.importCarSet(key,source);
  assert.equal(service.state.mods.length,count);assert.equal(project.revision,before.revision+1);
  assert.equal(project.changes.length,4);assert.equal(project.changes.filter(c=>c.field==='mesh').length,3);
  assert.equal(project.changes.find(c=>c.field==='texture').id,'car.0.material.0');
  const changes=project.changes;
  const appearance=await require('../main/preview-materials.cjs')(service,key,{materials:[{source:0,page:11,clut:22}]},null,new AbortController().signal);
  assert.equal(appearance[0].texture.png,png.toString('base64'));
  project=await service.projectHistory('undo',project.revision);assert.equal(project.changes.length,0);
  project=await service.projectHistory('redo',project.revision);assert.deepEqual(project.changes,changes);
  const originalTool=service.carTool;
  service.carTool=async args=>{
   const output=args.find(a=>a.startsWith('tools.car_output='));
   if(output){
    const model=changes.find(c=>c.field==='mesh');
    const bytes=await new (require('../main/project-files.cjs').ProjectFiles)(service.editor.file).read(model.value);
    await fs.writeFile(output.slice('tools.car_output='.length),bytes);
   }
   return originalTool(args);
  };
  const exported=await service.exportCarSet(key,root);
  const preview=await service.previewCar(key);
  assert.equal(preview.materials[0].texture.png,png.toString('base64'));
  assert.equal(preview.vertices.length,27);
  const exportedManifest=JSON.parse(await fs.readFile(path.join(exported,'car-set.json')));
  assert.deepEqual(await fs.readFile(path.join(exported,exportedManifest.textures[0])),png);
  for(const role of ['body','front-wheels','rear-wheels'])assert.match(await fs.readFile(path.join(exported,exportedManifest.parts[role]),'utf8'),/usemtl rage_0/);
  const exportedProject=await service.importCarSet(key,exported);
  assert.deepEqual(exportedProject.changes,changes); // Native OBJ/PNG round trip.
  project=await service.applyProject(project.revision);
  const pointer=JSON.parse(await fs.readFile(path.join(service.modsRoot,'active.json')));
  const {run}=require('../main/service.cjs');
  const applied=JSON.parse(await run(service.tool('rage-mod-cli'),[path.join(service.modsRoot,'builds',pointer.build,'mod.toml')]));
  assert.equal(Object.keys(applied.meshes).length,3);assert.ok(applied.textures['car.0.material.0'].startsWith('textures/project/'));
  await fs.writeFile(path.join(source,'image.png'),png.subarray(0,33));
  await assert.rejects(service.importCarSet(key,source),/Cannot import texture/);
  assert.deepEqual((await service.project()).changes,changes);
  assert.equal((await service.project()).revision,project.revision);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
