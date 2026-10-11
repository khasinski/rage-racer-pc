const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),os=require('node:os'),path=require('node:path');
const {LauncherService,run}=require('../main/service.cjs');
const {activeProfile}=require('../main/project-build.cjs');
const repo=path.resolve(__dirname,'../..');
test('apply compiles car deltas, atomically activates for launch, and retains the old build on failure',async()=>{
  const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-project-build-'));
  const rename=fs.rename;
  try{
    const bin=path.join(root,'bin');await fs.mkdir(bin);
    const native='rage-mod-cli'+(process.platform==='win32'?'.exe':'');
    await fs.copyFile(path.join(repo,process.env.RAGE_LAUNCHER_BUILD_DIR||'build/release',native),path.join(bin,native));
    for(const file of ['cars.toml','cars.ntscj.toml'])await fs.copyFile(path.join(repo,file),path.join(bin,file));
    const archive=path.join(root,'RAGE.BIN'),config=path.join(root,'default.ini');
    await fs.writeFile(archive,'source revision one');await fs.writeFile(config,'[video]\nrenderer = modern\n');
    const s=new LauncherService({root,bin,config});s.requireReady=()=>{};s.state.disc={archive,region:'PAL',path:archive,data:root};
    let p=await s.project();
    p=await s.editProject([{kind:'car',id:'car_00_g0',field:'price',value:0},{kind:'car',id:'car_00_g0',field:'manual_only',value:true}],p.revision);
    p=await s.applyProject(p.revision);assert.equal(p.dirty,false);
    const profile=await activeProfile(s),dir=path.dirname(s.editor.file),pointer=path.join(dir,'active.json');
    const oldPointer=await fs.readFile(pointer,'utf8');
    const {cars}=JSON.parse(await run(s.tool('rage-mod-cli'),['--car-profile',profile]));
    assert.equal(cars[0].price,0);assert.equal(cars[0].manual_only,true);
    assert.equal(cars[0].shift_points.length,12);
    assert.ok((await s.configuration()).includes('profile = '+profile));
    // Save-only leaves the last applied build untouched.
    p=await s.editProject([{kind:'car',id:'car_00_g0',field:'price',value:123}],p.revision);
    await s.saveProject(p.revision);assert.equal(await activeProfile(s),profile);
    fs.rename=async(a,b)=>{if(b===pointer)throw Object.assign(Error('simulated pointer failure'),{code:'EIO'});return rename(a,b);};
    await assert.rejects(s.applyProject(p.revision),/pointer failure/);fs.rename=rename;
    assert.equal(await fs.readFile(pointer,'utf8'),oldPointer);assert.equal(await activeProfile(s),profile);
    assert.equal((await fs.readdir(path.join(dir,'builds'))).length,1);
    // All-or-nothing even when another screen introduces unsupported edits.
    p=await s.editProject([{kind:'track',id:'coast',field:'road',value:1}],p.revision);
    await assert.rejects(s.applyProject(p.revision),/not connected/);assert.equal(await activeProfile(s),profile);
    p=await s.editProject([{kind:'track',id:'coast',field:'road',value:null},{kind:'car',id:'car_00_g0',field:'price',value:-1}],p.revision);
    await assert.rejects(s.applyProject(p.revision),/metadata/);assert.equal(await activeProfile(s),profile);
    await fs.appendFile(profile,'\n# tampered\n');
    await assert.rejects(s.configuration(),/damaged/);
    await fs.writeFile(archive,'source revision two');
    await assert.rejects(s.applyProject(p.revision),/Source archive changed/);
  }finally{fs.rename=rename;await fs.rm(root,{recursive:true,force:true});}
});
