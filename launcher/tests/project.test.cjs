const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),os=require('node:os'),path=require('node:path');
const {Project}=require('../main/project.cjs');
const source={region:'PAL',fingerprint:'sha256:'+'a'.repeat(64)};
const change=(kind,id,field,value)=>({kind,id,field,value});
test('one project history groups car and track edits and prevents stale updates',()=>{
  const p=new Project(source);
  const edits=[change('car','istante/grade/4','price',0),change('track','mythical-coast/class/1','road/0/left-width',400)];
  const s=p.edit(edits,0);assert.equal(s.changes.length,2);assert.equal(s.dirty,true);
  edits[0].value=100; s.changes[0].value=100;assert.equal(p.snapshot().changes[0].value,0);
  assert.throws(()=>p.edit(edits,0),/changed/);
  assert.equal(p.history('undo',1).changes.length,0);assert.equal(p.snapshot().dirty,false);
  assert.equal(p.history('redo',2).changes.length,2);
  assert.equal(p.edit([change('car','istante/grade/4','price',null)],3).changes.length,1);
  assert.equal(p.snapshot().canRedo,false);
});
test('invalid batch does not partially modify project or history',()=>{
  const p=new Project(source);
  for(const value of [NaN,Infinity,{},[],undefined])assert.throws(()=>p.edit([change('car','istante','price',0),change('car','istante','price',value)],0));
  assert.throws(()=>p.edit([change('car','__proto__/x','price',0)],0));
  assert.equal(p.snapshot().changes.length,0);assert.equal(p.revision,0);
});
test('save and reopen preserve deltas, enforce exact source and retain dirty state on write failure',async()=>{
  const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-project-'));
  try{
    const file=path.join(root,'project.rrproject'),p=new Project(source);
    p.edit([change('car','istante','manual-only',false)],0);
    await p.save(file);assert.equal(p.snapshot().dirty,false);
    const loaded=await Project.open(file,source);assert.deepEqual(loaded.snapshot().changes,p.snapshot().changes);
    await assert.rejects(Project.open(file,{...source,fingerprint:'sha256:'+'b'.repeat(64)}),/different disc revision/);
    p.edit([change('car','istante','price',0)],1);
    await assert.rejects(p.save(root));assert.equal(p.snapshot().dirty,true);
    assert.deepEqual((await Project.open(file,source)).snapshot().changes,loaded.snapshot().changes);
    assert.equal((await fs.readdir(root)).filter(n=>n.endsWith('.tmp')).length,0);
    const d=JSON.parse(await fs.readFile(file));d.changes.push(d.changes[0]);await fs.writeFile(file,JSON.stringify(d));
    await assert.rejects(Project.open(file,source),/Duplicate/);
  }finally{await fs.rm(root,{recursive:true,force:true});}
});
