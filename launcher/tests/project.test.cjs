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
test('array parameter edits copy caller data and participate in undo without mutation',()=>{
  const p=new Project(source),points=Array.from({length:12},(_,i)=>i);
  p.edit([change('car','istante','shift_points',points)],0);points[0]=999;
  assert.equal(p.snapshot().changes[0].value[0],0);
  p.history('undo',1);assert.equal(p.snapshot().changes.length,0);
  p.history('redo',2);assert.equal(p.snapshot().changes[0].value[0],0);
});
test('package choices and conflict priorities participate in the same history as car and track edits',()=>{
  const p=new Project(source),selected={packages:['b','a'],resolutions:{'mesh:car.player.10.part.0':{winner:'b',candidates:['b','a']}}};
  p.edit([change('car','istante','price',0)],0);
  p.select(selected,1);selected.packages.length=0;selected.resolutions['mesh:car.player.10.part.0'].winner='a';
  assert.deepEqual(p.snapshot().selection.packages,['a','b']);
  assert.equal(p.snapshot().selection.resolutions['mesh:car.player.10.part.0'].winner,'b');
  p.edit([change('track','coast','width',500)],2);
  p.history('undo',3);assert.equal(p.snapshot().changes.length,1);assert.equal(p.snapshot().selection.packages.length,2);
  p.history('undo',4);assert.deepEqual(p.snapshot().selection,{packages:[],resolutions:{}});assert.equal(p.snapshot().changes.length,1);
  assert.equal(p.usesPackage('a'),true);
  p.history('redo',5);assert.equal(p.snapshot().selection.resolutions['mesh:car.player.10.part.0'].winner,'b');
  const reordered={packages:['b','a'],resolutions:{'mesh:car.player.10.part.0':{winner:'b',candidates:['b','a']}}};
  assert.equal(p.select(reordered,6).revision,6); // canonical order is a no-op
  assert.throws(()=>p.select(reordered,0),/changed/);
});
test('invalid selections and oversized document edits leave the draft and history untouched',()=>{
  const p=new Project(source),before=p.snapshot();
  for(const bad of [
    null,{packages:['a','a'],resolutions:{}},{packages:['../escape'],resolutions:{}},
    {packages:['a'],resolutions:[]},{packages:[],resolutions:{constructor:{winner:'a',candidates:['a','b']}}},
    {packages:[],resolutions:{'mesh:car.0':{winner:'c',candidates:['a','b']}}},
    {packages:[],resolutions:{'textures/../escape.png':{winner:'a',candidates:['a','b']}}},
  ])assert.throws(()=>p.select(bad,0),/Invalid project/);
  const large=Array.from({length:2200},(_,i)=>change('car','car'+i,'description','x'.repeat(4096)));
  assert.throws(()=>p.edit(large,0),/size limit/);
  assert.deepEqual(p.snapshot(),before);
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
test('saved selections reopen exactly and old car-only documents adopt legacy selection once',async()=>{
  const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-project-selection-'));
  try{
    const file=path.join(root,'project.rrproject'),p=new Project(source);
    const selected={packages:['a','b'],resolutions:{'material:car.0.material.0':{winner:'a',candidates:['a','b']}}};
    p.select(selected,0);await p.save(file);
    const opened=await Project.open(file,source,{packages:['other'],resolutions:{}});
    assert.deepEqual(opened.snapshot().selection,selected);assert.equal(opened.snapshot().dirty,false);
    const old=JSON.parse(await fs.readFile(file));old.format=1;delete old.selection;await fs.writeFile(file,JSON.stringify(old));
    const migrated=await Project.open(file,source,selected);assert.equal(migrated.snapshot().format,2);
    assert.deepEqual(migrated.snapshot().selection,selected);await migrated.save(file);
    assert.deepEqual((await Project.open(file,source)).snapshot().selection,selected);
    const broken=JSON.parse(await fs.readFile(file));broken.selection.packages.push('a');await fs.writeFile(file,JSON.stringify(broken));
    await assert.rejects(Project.open(file,source),/Invalid project package selection/);
    delete broken.selection;await fs.writeFile(file,JSON.stringify(broken));
    await assert.rejects(Project.open(file,source),/Invalid project package selection/);
    broken.selection=selected;broken.id='-'.repeat(36);await fs.writeFile(file,JSON.stringify(broken));
    await assert.rejects(Project.open(file,source),/Invalid project document/);
  }finally{await fs.rm(root,{recursive:true,force:true});}
});
test('large edit history evicts distant states while retaining working undo and redo',()=>{
  const p=new Project(source);
  for(let revision=0;revision<12;revision++){
    const value=String(revision).padEnd(4096,'.');
    p.edit(Array.from({length:1000},(_,i)=>change('car','car'+i,'description',value)),p.revision);
  }
  let state=p.snapshot(),undos=0;
  while(state.canUndo){state=p.history('undo',state.revision);undos++;}
  assert.ok(undos>0&&undos<12);assert.equal(state.changes.length,1000);
  assert.equal(state.canRedo,true);
  const value=state.changes[0].value;
  state=p.history('redo',state.revision);assert.notEqual(state.changes[0].value,value);
});
