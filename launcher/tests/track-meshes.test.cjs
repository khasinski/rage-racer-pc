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
