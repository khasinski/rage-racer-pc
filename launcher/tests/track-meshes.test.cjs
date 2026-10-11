const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {Project}=require('../main/project.cjs'),{ProjectFiles}=require('../main/project-files.cjs'),{model,slot}=require('../main/track-meshes.cjs');
function mesh(count=1,x=1){const b=Buffer.alloc(24+(count+1)*4+120+12);b.write('RRMESH1');b.writeUInt32LE(1,8);b.writeUInt32LE(count,12);b.writeUInt32LE(3,16);b.writeUInt32LE(3,20);for(let i=1;i<=count;i++)b.writeUInt32LE(3,24+i*4);const at=24+(count+1)*4;for(let i=0;i<3;i++){b.writeFloatLE(x+i,at+i*40);b.writeFloatLE(1,at+i*40+20);b.fill(128,at+i*40+24,at+i*40+28);b.writeUInt32LE(65535,at+i*40+36);b.writeUInt32LE(i,at+120+i*4);}return b;}
test('track preview resolves immutable project geometry over package and rejects mismatched banks',async t=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-track-mesh-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));
 const file=path.join(root,'project.rrproject'),input=path.join(root,'input.rmesh'),target=path.join(root,'course.rmesh');
 const project=new Project({region:'PAL',fingerprint:'sha256:'+'a'.repeat(64)}),service={editor:{file,project},tool:n=>path.resolve(__dirname,'../resources/bin',n)};
 await fs.mkdir(path.join(root,'meshes'));await fs.writeFile(path.join(root,'meshes/package.rmesh'),mesh(1,2));await fs.writeFile(path.join(root,'mod.toml'),'[mod]\nid="track"\n[meshes]\n"track.big1.course"="meshes/package.rmesh"\n');
 await fs.writeFile(target,mesh());await model(service,'track.big1.course',target,root,['mod.toml','meshes/package.rmesh'],undefined,1);assert.deepEqual(await fs.readFile(target),mesh(1,2));
 await fs.writeFile(input,mesh(1,3));const ref=await new ProjectFiles(file).put(input);project.edit([{kind:'track',id:'track.big1.course',field:'mesh',value:ref}],project.revision);
 await model(service,'track.big1.course',target,root,['mod.toml','meshes/package.rmesh'],undefined,1);assert.deepEqual(await fs.readFile(target),mesh(1,3));
 project.history('undo',project.revision);await model(service,'track.big1.course',target,root,['mod.toml','meshes/package.rmesh'],undefined,1);assert.deepEqual(await fs.readFile(target),mesh(1,2));
 await fs.writeFile(input,mesh(2));const bad=await new ProjectFiles(file).put(input);project.edit([{kind:'track',id:'track.big1.course',field:'mesh',value:bad}],project.revision);
 await assert.rejects(model(service,'track.big1.course',target,root,[],undefined,1),/retain its model count/);
 assert.deepEqual(slot('track.oval6.terrain'),{bank:134,name:'terrain'});assert.throws(()=>slot('track.big7.course'),/Invalid/);
});
test('bank import contract accepts original and untextured materials and rejects unknown bindings',()=>{
 const {checkBank}=require('../main/track-meshes.cjs'),metadata={meshes:64,materials:[{source:0},{source:130}]};
 assert.doesNotThrow(()=>checkBank({meshes:64,materials:[0,130,65535]},metadata));
 assert.throws(()=>checkBank({meshes:63,materials:[0]},metadata),/model count/);
 assert.throws(()=>checkBank({meshes:64,materials:[131]},metadata),/unknown material slots/);
});
test('compiled replacement preserves other bank models and never overwrites an existing output',async t=>{
 const {run}=require('../main/service.cjs'),root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-track-replace-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));
 const base=path.join(root,'base.rmesh'),part=path.join(root,'part.rmesh'),out=path.join(root,'out.rmesh'),tool=path.resolve(__dirname,'../resources/bin/rage-mod-cli');
 await fs.writeFile(base,mesh(2,1));await fs.writeFile(part,mesh(1,9));
 await run(tool,['--replace-mesh',base,'1',part,out]);
 const preview=JSON.parse(await run(tool,['--preview-bank',out]));assert.deepEqual(preview.ranges,[[0,3],[3,3]]);assert.equal(preview.vertices[0],1);assert.equal(preview.vertices[27],9);
 const saved=await fs.readFile(out);await assert.rejects(run(tool,['--replace-mesh',base,'0',part,out]),/Invalid mesh replacement/);assert.deepEqual(await fs.readFile(out),saved);
 await fs.rm(out);await assert.rejects(run(tool,['--replace-mesh',base,'2',part,out]),/Invalid mesh replacement/);await assert.rejects(fs.stat(out),{code:'ENOENT'});
 assert.deepEqual(await fs.readFile(base),mesh(2,1));
});
test('track model textures validate source, fixed paths, decoding and immutable references',async t=>{
 const {importTextures}=require('../main/track-meshes.cjs'),root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-track-png-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));
 const source={region:'PAL',fingerprint:'sha256:'+'a'.repeat(64)},project=new Project(source),file=path.join(root,'project.rrproject'),service={editor:{file,project},tool:n=>path.resolve(__dirname,'../resources/bin',n)},input=path.join(root,'model.obj'),metadata={materials:[{source:2}]};
 const png=Buffer.from('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVQIHWP4z8DwHwAFgAI/ScLttAAAAABJRU5ErkJggg==','base64'),image=path.join(root,'texture-2.png');await fs.writeFile(image,png);
 const manifest={format:1,id:'track.big1.course',part:55,source,textures:{2:'texture-2.png'}},sidecar=path.join(root,'track-model.json');
 await fs.writeFile(sidecar,JSON.stringify(manifest));const changes=await importTextures(service,manifest.id,55,input,metadata,undefined,root);assert.deepEqual(await fs.readFile(path.join(root,'import-texture-2.png')),png);
 assert.equal(changes[0].id,'track.big1.course.material.2');assert.equal(changes[0].kind,'track');assert.deepEqual(await new ProjectFiles(file).read(changes[0].value),png);
 await assert.rejects(importTextures(service,manifest.id,54,input,metadata),/does not match/);
 await fs.writeFile(sidecar,JSON.stringify({...manifest,textures:{2:'../image.png'}}));await assert.rejects(importTextures(service,manifest.id,55,input,metadata),/Invalid track texture binding/);
 await fs.writeFile(sidecar,JSON.stringify(manifest));await fs.writeFile(image,png.subarray(0,33));await assert.rejects(importTextures(service,manifest.id,55,input,metadata));
 assert.equal(project.revision,0);
});
