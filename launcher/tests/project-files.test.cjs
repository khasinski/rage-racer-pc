const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs/promises'),os=require('node:os'),path=require('node:path');
const {ProjectFiles}=require('../main/project-files.cjs');
const {Project}=require('../main/project.cjs');

test('immutable model references survive shared undo, redo and reopening',async t=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-project-files-'));
 t.after(()=>fs.rm(root,{recursive:true,force:true}));
 const file=path.join(root,'project.rrproject'),input=path.join(root,'input.rmesh');
 const files=new ProjectFiles(file),source={region:'PAL',fingerprint:'sha256:'+'a'.repeat(64)},project=new Project(source);
 await fs.writeFile(input,'first validated model');const first=await files.put(input);
 await fs.writeFile(input,'second validated model');const second=await files.put(input);
 const edit=value=>project.edit([{kind:'car',id:'car.player.11.part.0',field:'mesh',value}],project.revision);
 edit(first);edit(second);project.history('undo',project.revision);
 assert.equal(project.snapshot().changes[0].value,first);
 assert.equal((await files.read(first)).toString(),'first validated model');
 project.history('redo',project.revision);await project.save(file);
 assert.equal((await Project.open(file,source)).snapshot().changes[0].value,second);
 assert.equal(await files.put(input),second);
 assert.equal((await fs.readdir(files.root)).length,2);
 edit(null);assert.deepEqual(project.snapshot().changes,[]);
 assert.equal((await files.read(second)).toString(),'second validated model');
});

test('project model store rejects corrupt objects, traversal and symlinks',async t=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-project-files-'));
 t.after(()=>fs.rm(root,{recursive:true,force:true}));
 const files=new ProjectFiles(path.join(root,'project.rrproject')),input=path.join(root,'input');
 await fs.writeFile(input,'model');const ref=await files.put(input),stored=path.join(files.root,ref.slice(7));
 await assert.rejects(files.read('../input'),/Invalid project model reference/);
 await fs.chmod(stored,0o644);await fs.writeFile(stored,'corrupt');
 await assert.rejects(files.read(ref),/changed or is damaged/);
 await assert.rejects(files.put(input),/changed or is damaged/);
 await fs.rm(stored);await fs.symlink(input,stored);
 await assert.rejects(files.read(ref),/Invalid project model file/);
 assert.deepEqual(await fs.readdir(files.root),[ref.slice(7)]);
});
