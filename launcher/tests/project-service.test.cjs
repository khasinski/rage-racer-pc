const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),os=require('node:os'),path=require('node:path');
const {LauncherService}=require('../main/service.cjs');

test('editor persists a shared source-specific project and never inherits it after changing discs',async()=>{
  const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-project-service-'));
  try{
    const archive=path.join(root,'RAGE.BIN');await fs.writeFile(archive,'source revision one');
    const service=new LauncherService({root,bin:root,config:''});service.requireReady=()=>{};
    service.state.disc={archive,region:'NTSC-J'};
    const original=await service.project();assert.match(original.source.fingerprint,/^sha256:[a-f0-9]{64}$/);
    const edits=await service.editProject([{kind:'car',id:'istante/grade/4',field:'price',value:0}],original.revision);
    await service.saveProject(edits.revision);
    assert.equal(service.state.mods.length,0); // no separate mod per document edit
    const reopened=new LauncherService({root,bin:root,config:''});reopened.requireReady=()=>{};reopened.state.disc=service.state.disc;
    assert.deepEqual((await reopened.project()).changes,edits.changes);
    await fs.writeFile(archive,'source revision two');
    reopened.state.disc={archive,region:'NTSC-J'};
    await assert.rejects(reopened.editProject([],0),/Open the project/);
    const fresh=await reopened.project();assert.notEqual(fresh.source.fingerprint,original.source.fingerprint);assert.equal(fresh.changes.length,0);
  }finally{await fs.rm(root,{recursive:true,force:true});}
});
test('package enablement and conflict choices use project history, restore after reopening and roll back failed persistence',async()=>{
  const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-project-packages-'));
  try{
    const archive=path.join(root,'RAGE.BIN');await fs.writeFile(archive,'source revision');
    const options={root:path.join(root,'editor'),bin:path.resolve(__dirname,'../resources/bin'),config:path.resolve(__dirname,'../resources/rage-port.ini')};
    const s=new LauncherService(options);await s.init();s.state.disc={archive,region:'PAL'};
    let p=await s.project();
    async function install(name,metallic){
      const folder=path.join(root,name);await fs.mkdir(folder);
      await fs.writeFile(path.join(folder,'mod.toml'),`[materials]\n"car.0.material.0" = "lit opaque 0.3 ${metallic} 1 1 1 1 0 0 0"\n`);
      return s.importMod(folder);
    }
    const a=await install('first',0),b=await install('second',1);
    await s.toggleMod(a.id,true);p=(await s.snapshot()).project;
    assert.equal(p.revision,1);assert.equal(p.dirty,true);assert.deepEqual(p.selection.packages,[a.id]);
    p=await s.editProject([{kind:'car',id:'car_00_g0',field:'price',value:0}],p.revision);
    await s.toggleMod(b.id,true);p=(await s.snapshot()).project;assert.equal(p.revision,3);
    const key=s.overlaps()[0].key;assert.equal(s.conflicts().length,1);
    await s.resolveConflict(key,b.id);p=(await s.snapshot()).project;
    assert.equal(p.revision,4);assert.equal(p.selection.resolutions[key].winner,b.id);
    await s.saveProject(p.revision);assert.equal((await s.snapshot()).project.dirty,false);
    await s.resolveConflict(key,a.id);p=(await s.snapshot()).project;
    p=await s.projectHistory('undo',p.revision);assert.equal(s.state.resolutions[key].winner,b.id);assert.equal(p.dirty,false);
    p=await s.projectHistory('undo',p.revision);assert.equal(s.state.resolutions[key],undefined);assert.equal(s.conflicts().length,1);
    p=await s.projectHistory('undo',p.revision);assert.equal(b.enabled,false);assert.equal(a.enabled,true);assert.equal(p.changes[0].value,0);
    p=await s.projectHistory('redo',p.revision);assert.equal(b.enabled,true);
    p=await s.projectHistory('redo',p.revision);assert.equal(s.state.resolutions[key].winner,b.id);
    p=await s.projectHistory('redo',p.revision);assert.equal(s.state.resolutions[key].winner,a.id);
    const before=p,persist=s.persist;
    s.persist=async()=>{throw Error('Disk full');};
    await assert.rejects(s.toggleMod(a.id,false),/Disk full/);
    assert.deepEqual((await s.snapshot()).project,before);assert.equal(a.enabled,true);assert.equal(s.state.resolutions[key].winner,a.id);
    s.persist=persist;
    await assert.rejects(s.editProjectSelection({packages:[],resolutions:{}},0),/changed/);
    await assert.rejects(s.removeMod(a.id),/undo history/);
    // The saved document, not the newer launcher cache, owns restored choices.
    const reopened=new LauncherService(options);await reopened.init();
    const restored=(await reopened.snapshot()).project;
    assert.deepEqual(restored.selection.packages,[a.id,b.id].sort());
    assert.equal(restored.selection.resolutions[key].winner,b.id);
    assert.equal(reopened.state.mods.find(m=>m.id===a.id).enabled,true);
    assert.equal(reopened.state.resolutions[key].winner,b.id);
    assert.equal(restored.changes[0].value,0);
    // Missing libraries remain named selections and block a build explicitly.
    reopened.state.mods=reopened.state.mods.filter(m=>m.id!==a.id);
    assert.throws(()=>reopened.modsForSelection(restored.selection),/Project package is unavailable/);
    const cleared=await reopened.editProjectSelection({packages:[],resolutions:{}},restored.revision);
    assert.deepEqual(cleared.selection.packages,[]);assert.equal(cleared.canUndo,true);
    const undone=await reopened.projectHistory('undo',cleared.revision);
    assert.deepEqual(undone.selection,restored.selection);
  }finally{await fs.rm(root,{recursive:true,force:true});}
});
