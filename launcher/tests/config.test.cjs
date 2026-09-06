const {test}=require('node:test');const assert=require('node:assert/strict');
const {defaults,validate,patchIni}=require('../main/config.cjs');
test('launch configuration forces modern and selected regional content while preserving user settings',async()=>{
 const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
 const {LauncherService}=require('../main/service.cjs');
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-launch-config-'));
 try{
  const file=path.join(root,'rage-port.ini');
  const original='# user settings\n[video]\nrenderer=classic\ninternal_scale=3\n[video]\nrenderer=classic\n[diagnostics]\nmarker_capture=true\n[custom]\nkeep=yes\n';
  await fs.writeFile(file,original);
  const service=new LauncherService({root,config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();service.state.settings['video.internal_scale']='4';
  service.state.settings['diagnostics.marker_capture']=true;
  for(const region of ['PAL','NTSC-U','NTSC-J']){
   service.state.disc={region,path:path.join(root,'selected image.bin')};
   const config=await service.configuration(path.join(root,'active mods'));
   assert.equal((config.match(/renderer = modern/g)||[]).length,2);
   assert.doesNotMatch(config,/renderer\s*=\s*classic/);
   assert.match(config,/internal_scale = 4/);
   assert.match(config,/marker_capture = true/);assert.match(config,/keep=yes/);
   const style=region==='NTSC-J'?'japanese':'international';
   assert.ok(config.includes('car_names = '+style));assert.ok(config.includes('prologue = '+style));
   assert.ok(config.includes('image = '+service.state.disc.path));
   assert.match(config,/choose = false/);
   assert.ok(config.includes('directory = '+path.join(root,'active mods')));
  }
  assert.equal(await fs.readFile(file,'utf8'),original);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
test('settings reject unknown options, injected INI lines and out-of-range values',()=>{
 assert.deepEqual(validate(defaults),defaults);
 for(const input of [{'content.car_names':'japanese'},{'video.internal_scale':'NaN'},{'video.internal_scale':'17'},{'video.aspect':'16:9\n[tools]'},{'input.analog':'false'}])assert.throws(()=>validate(input));
});
test('configuration retains unrelated settings and updates duplicate known keys',()=>{
 const source='# keep this\n[video]\ninternal_scale=2\nunknown=retain\n[video]\ninternal_scale=3\n[custom]\nthing=yes\n';
 const result=patchIni(source,{'video.internal_scale':'4','hud.anchor':'edges'});
 assert.equal((result.match(/internal_scale = 4/g)||[]).length,2);
 assert.match(result,/# keep this/);assert.match(result,/unknown=retain/);assert.match(result,/thing=yes/);assert.match(result,/\[hud\]\nanchor = edges/);
 assert.throws(()=>patchIni('',{'disc.image':'a\nb'}));
});
