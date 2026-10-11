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
