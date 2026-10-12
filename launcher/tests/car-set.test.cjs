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
  const materialList=await service.carMaterials(key);
  assert.equal(materialList.revision,project.revision);assert.equal(materialList.materials[0].id,'car.0.material.0');
  assert.equal(materialList.materials[0].name,'Texture 1');
  const properties='lit opaque 0.7 0.2 1 0.5 0.5 1 0 0 0';
  project=await service.saveMaterial('',materialList.materials[0].id,properties,project.revision);
  assert.match((await service.carMaterials(key)).materials[0].properties,/0.7/);
  project=await service.projectHistory('undo',project.revision);assert.deepEqual(project.changes,changes);
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
  const whole=await fs.readFile(path.join(exported,'car.glb')),jsonLength=whole.readUInt32LE(12),glb=JSON.parse(whole.subarray(20,20+jsonLength).toString());
  assert.equal(glb.meshes.length,3);assert.equal(glb.nodes.length,5);
  assert.deepEqual(glb.extras.rr_source,{region:'NTSC-U',key,fingerprint:service.editor.project.document.source.fingerprint});
  assert.deepEqual(glb.nodes.slice(1).map(n=>n.mesh),[0,2,1,1]);
  assert.deepEqual(glb.nodes[4].scale,[-1,1,-1]);
  assert.deepEqual(glb.meshes.map(m=>m.extras.rr_part),[key,'car.player.10.part.2','car.player.10.part.3']);
  const bin=whole.subarray(28+jsonLength);
  for(const image of glb.images){const v=glb.bufferViews[image.bufferView];assert.deepEqual(bin.subarray(v.byteOffset,v.byteOffset+v.byteLength),png);}
  for(const mesh of glb.meshes)for(const primitive of mesh.primitives){
   const accessor=glb.accessors[primitive.attributes.POSITION],v=glb.bufferViews[accessor.bufferView];
   assert.ok(v.byteOffset+v.byteLength<=bin.length);assert.equal(accessor.count,3);
   assert.equal(glb.materials[primitive.material].extras.rr_material,0);
   const texture=glb.materials[primitive.material].pbrMetallicRoughness.baseColorTexture;
   assert.ok(glb.images[glb.textures[texture.index].source]);
  }
  const preview=await service.previewCar(key);
  assert.equal(preview.materials[0].texture.png,png.toString('base64'));
  assert.equal(preview.vertices.length,27);
  const exportedManifest=JSON.parse(await fs.readFile(path.join(exported,'car-set.json')));
  assert.deepEqual(await fs.readFile(path.join(exported,exportedManifest.textures[0])),png);
  for(const role of ['body','front-wheels','rear-wheels'])assert.match(await fs.readFile(path.join(exported,exportedManifest.parts[role]),'utf8'),/usemtl rage_0/);
  const exportedProject=await service.importCarSet(key,exported);
  assert.deepEqual(exportedProject.changes,changes); // Native OBJ/PNG round trip.
  const wholeProject=await service.importCarSet(key,path.join(exported,'car.glb'));
  assert.deepEqual(wholeProject.changes,changes); // Complete embedded GLB round trip.
  const external=path.join(root,'external');await fs.mkdir(path.join(external,'textures'),{recursive:true});
  const externalDoc=structuredClone(glb);externalDoc.buffers[0].uri='geometry.bin';await fs.writeFile(path.join(external,'geometry.bin'),bin);
  for(const [i,image]of externalDoc.images.entries()){const view=externalDoc.bufferViews[image.bufferView];await fs.writeFile(path.join(external,'textures',i+'.png'),bin.subarray(view.byteOffset,view.byteOffset+view.byteLength));delete image.bufferView;delete image.mimeType;image.uri='textures/'+i+'.png';}
  const externalFile=path.join(external,'car.gltf'),externalText=JSON.stringify(externalDoc);await fs.writeFile(externalFile,externalText);
  assert.deepEqual((await service.importCarSet(key,externalFile)).changes,changes);assert.equal(await fs.readFile(externalFile,'utf8'),externalText);
  const externalCandidate=await service.prepareCarImport(key,externalFile);await fs.writeFile(path.join(external,'geometry.bin'),'changed after selection');
  const externalMapping={nodes:externalCandidate.nodes.map(n=>({node:n.node,instance:glb.nodes[n.node].extras.rr_instance})),materials:externalCandidate.materials.map(m=>({material:m.material,word:glb.materials[m.material].extras.rr_material}))};
  assert.deepEqual((await service.completeCarImport(externalCandidate.token,externalMapping)).changes,changes);await fs.writeFile(path.join(external,'geometry.bin'),bin);
  const edited=Buffer.from(whole),bodyPosition=glb.accessors[glb.meshes[0].primitives[0].attributes.POSITION],bodyView=glb.bufferViews[bodyPosition.bufferView],vertexAt=28+jsonLength+bodyView.byteOffset+(bodyPosition.byteOffset||0);
  edited.writeFloatLE(7,vertexAt);const editedFile=path.join(root,'edited.glb');await fs.writeFile(editedFile,edited);
  let editedProject=await service.importCarSet(key,editedFile);
  assert.equal(editedProject.revision,wholeProject.revision+1);
  const bodyChange=editedProject.changes.find(c=>c.id===key&&c.field==='mesh'),bodyBytes=await new (require('../main/project-files.cjs').ProjectFiles)(service.editor.file).read(bodyChange.value);
  assert.equal(bodyBytes.readFloatLE(32),7);assert.equal(editedProject.changes.length,changes.length);
  project=await service.projectHistory('undo',editedProject.revision);assert.deepEqual(project.changes,changes);
  const rewrite=edit=>{const doc=JSON.parse(whole.subarray(20,20+jsonLength));edit(doc);const text=Buffer.from(JSON.stringify(doc)),padded=Buffer.alloc((text.length+3)&~3,32);text.copy(padded);const rest=whole.subarray(20+jsonLength),file=Buffer.alloc(20+padded.length+rest.length);whole.copy(file,0,0,12);file.writeUInt32LE(file.length,8);file.writeUInt32LE(padded.length,12);file.write('JSON',16);padded.copy(file,20);rest.copy(file,20+padded.length);return file;};
  const nodeProperties=path.join(root,'node-properties.glb');await fs.writeFile(nodeProperties,rewrite(d=>{delete d.extras;}));assert.deepEqual((await service.importCarSet(key,nodeProperties)).changes,changes);
  const lost=path.join(root,'lost-properties.glb'),lostBytes=rewrite(d=>{delete d.extras;for(const n of d.nodes)delete n.extras;for(const m of d.meshes)delete m.extras;for(const m of d.materials){delete m.extras;m.name='Edited material';}});await fs.writeFile(lost,lostBytes);
  const pending=await service.prepareCarImport(key,lost);assert.equal(pending.nodes.length,4);assert.equal(pending.roles.length,4);assert.equal((await service.project()).revision,project.revision);
  const candidatePreview=await service.previewCarImport(pending.token,1);assert.equal(candidatePreview.indices.length,3);assert.equal(candidatePreview.vertices[9],1);assert.equal(candidatePreview.textures[0].png,png.toString('base64'));
  const mirroredPreview=await service.previewCarImport(pending.token,4);assert.ok(mirroredPreview.vertices.some((v,i)=>i%9===0&&v===-1));
  assert.deepEqual(await service.previewCarImport(pending.token,1),candidatePreview);assert.equal((await service.project()).revision,project.revision);
  await assert.rejects(service.previewCarImport(pending.token,999),/Choose a part/);
  const targetTexture=await service.carImportTexture(pending.token,0);assert.equal(targetTexture.png,png.toString('base64'));assert.equal((await service.project()).revision,project.revision);
  await assert.rejects(service.carImportTexture(pending.token,999999),/Choose a material/);
  const mapping={nodes:pending.nodes.map(n=>({node:n.node,instance:glb.nodes[n.node].extras.rr_instance})),materials:pending.materials.map(m=>({material:m.material,word:0}))};
  await assert.rejects(service.completeCarImport(pending.token,{...mapping,nodes:mapping.nodes.map(n=>({...n,instance:0}))}),/different role/);
  await assert.rejects(service.completeCarImport(pending.token,{...mapping,materials:mapping.materials.map(m=>({...m,word:999999}))}),/original car materials/);
  assert.equal((await service.project()).revision,project.revision);
  assert.deepEqual((await service.completeCarImport(pending.token,mapping)).changes,changes);assert.equal(service.carImport,null);assert.deepEqual(await fs.readFile(lost),lostBytes);
  const cancelled=await service.prepareCarImport(key,lost);await service.cancelCarImport(cancelled.token);await assert.rejects(service.completeCarImport(cancelled.token,mapping),/expired/);
  const stale=await service.prepareCarImport(key,lost);project=await service.saveMaterial('',materialList.materials[0].id,properties,project.revision);project=await service.projectHistory('undo',project.revision);
  await assert.rejects(service.completeCarImport(stale.token,mapping),/Project changed/);assert.deepEqual((await service.project()).changes,changes);await service.cancelCarImport(stale.token);
  for(const label of ['Mapping car parts','Importing complete car','Previewing target material']){
   const active=await service.prepareCarImport(key,lost);let closing;
   service.onChange=state=>{if(state.busy===label)closing=service.cancelCarImport(active.token);};
   await assert.rejects(label==='Previewing target material'?service.carImportTexture(active.token,0):service.completeCarImport(active.token,mapping),/Operation canceled/);await closing;service.onChange=()=>{};
   assert.equal(service.carImport,null);assert.equal(service.busy,null);assert.equal((await service.project()).revision,project.revision);assert.deepEqual((await service.project()).changes,changes);
  }
  const late=await service.prepareCarImport(key,lost),Files=require('../main/project-files.cjs').ProjectFiles,put=Files.prototype.put;let lateClosing;
  Files.prototype.put=async function(...args){const value=await put.apply(this,args);if(!lateClosing)lateClosing=service.cancelCarImport(late.token);return value;};
  try{await assert.rejects(service.completeCarImport(late.token,mapping),/Operation canceled/);}finally{Files.prototype.put=put;}
  assert.ok(lateClosing);await lateClosing;assert.equal(service.carImport,null);assert.equal((await service.project()).revision,project.revision);assert.deepEqual((await service.project()).changes,changes);
  assert.equal((await fs.readdir(service.root)).some(n=>n.startsWith('car-map-')),false);
  for(const edit of [d=>{d.extras.rr_source.key='car.player.12.part.0';},d=>{d.extras.rr_source.fingerprint='sha256:'+'0'.repeat(64);},d=>{delete d.nodes[1].extras.rr_part;},d=>{d.nodes[4].translation[0]+=5;},d=>{d.nodes[1].scale=[0,1,1];}]){
   const bad=path.join(root,'invalid-car.glb');await fs.writeFile(bad,rewrite(edit));await assert.rejects(service.importCarSet(key,bad));assert.equal((await service.project()).revision,project.revision);assert.deepEqual((await service.project()).changes,changes);
  }
  const scaled=path.join(root,'scaled-wheels.glb');await fs.writeFile(scaled,rewrite(d=>{for(const i of [3,4])d.nodes[i].scale=d.nodes[i].scale.map(n=>n*2);}));
  editedProject=await service.importCarSet(key,scaled);
  const frontChange=editedProject.changes.find(c=>c.id==='car.player.10.part.2'&&c.field==='mesh'),frontBytes=await new (require('../main/project-files.cjs').ProjectFiles)(service.editor.file).read(frontChange.value);
  assert.equal(frontBytes.readFloatLE(32+40),2);assert.equal(frontBytes.readFloatLE(32+20),0.5);
  project=await service.projectHistory('undo',editedProject.revision);assert.deepEqual(project.changes,changes);
  const nan=Buffer.from(whole);nan.writeFloatLE(NaN,vertexAt);const nanFile=path.join(root,'nan.glb');await fs.writeFile(nanFile,nan);await assert.rejects(service.importCarSet(key,nanFile));assert.equal((await service.project()).revision,project.revision);
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
