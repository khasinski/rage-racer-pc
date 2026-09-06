const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {run,LauncherService}=require('../main/service.cjs');
const bin=path.resolve(__dirname,'../resources/bin');
const tool=path.join(bin,'rage-mod-cli'+(process.platform==='win32'?'.exe':''));
test('launcher resources include the exact vendored JSON parser license',async()=>{
 assert.deepEqual(await fs.readFile(path.resolve(__dirname,'../resources/licenses/yyjson.txt')),
  await fs.readFile(path.resolve(__dirname,'../../external/yyjson/LICENSE')));
});
test('native package metadata retains Unicode, extensions and launcher length limits',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-package-'));
 try{
  const file=path.join(root,'rage-mod.json');
  const base={format:1,name:'Żółć 🚗',region:'PAL',packageId:'Author.mod_1',author:'作者',description:'First\nSecond\tline',extension:{future:[1,true,null]}};
  async function check(value){const text=JSON.stringify(value);await fs.writeFile(file,text);return JSON.parse(await run(tool,['--metadata',file]));}
  assert.deepEqual(await check(base),base);
  assert.equal((await check({...base,name:'🚗'.repeat(100)})).name.length,200);
  assert.equal((await check({...base,description:'界'.repeat(1200)})).description.length,1200);
  const requires=Array.from({length:32},(_,i)=>({packageId:'dep'+i,version:'1.0'}));
  assert.deepEqual((await check({...base,requires})).requires,requires);
  for(const patch of [{name:'🚗'.repeat(101)},{name:'\u2003\ufeff'},{name:'\ud800'},
   {description:'x'.repeat(1201)},{author:'bad\n'},{version:'\0'},
   {region:'界'.repeat(7)},{packageId:'_bad'},{requires:[...requires,{packageId:'extra'}]},
   {requires:[{packageId:'base',version:'\u00a0'}]},{requires:[{packageId:'base',optional:true}]},
   {requires:[{packageId:base.packageId}]},{requires:[{packageId:'base'},{packageId:'base'}]}]){
   await assert.rejects(check({...base,...patch}),/Invalid mod metadata/);
  }
  for(const text of ['{"format":1,"format":1,"name":"x","region":"PAL"}',
   '{"format":1,"name":"x","region":"PAL"} trailing',
   JSON.stringify({...base,extension:'x'.repeat(16384)})]){
   await fs.writeFile(file,text);await assert.rejects(run(tool,['--metadata',file]),/Invalid mod metadata/);
  }
  await fs.writeFile(file,Buffer.from('{"format":1,"name":"\xff","region":"PAL"}','latin1'));
  await assert.rejects(run(tool,['--metadata',file]),/Invalid mod metadata/);
  await check(base);
  const service=new LauncherService({root:path.join(root,'profile'),bin,config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();service.state.disc={region:'PAL'};
  const source=path.join(root,'source');await fs.mkdir(source);
  await fs.copyFile(file,path.join(source,'rage-mod.json'));
  const installed=await service.importMod(source);
  assert.equal(installed.name,base.name);assert.equal(installed.author,base.author);
  assert.equal(installed.description,base.description);assert.equal(installed.packageId,base.packageId);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
