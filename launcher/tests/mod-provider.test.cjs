const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {run,LauncherService}=require('../main/service.cjs');
const bin=path.resolve(__dirname,'../resources/bin');
const tool=path.join(bin,'rage-mod-cli'+(process.platform==='win32'?'.exe':''));
const resolve=async args=>JSON.parse(await run(tool,['--resolve-providers',...args]));
test('native provider selection is explicit, order independent and batch fail-closed',async()=>{
 assert.deepEqual(await resolve([]),[]);
 const candidates=['--resource','texture:car.a','--candidate','a','--candidate','b'];
 await assert.rejects(resolve(candidates),/conflict.*no choice/);
 const choice=['--choice','b','--previous','b','--previous','a'];
 assert.deepEqual(await resolve([...candidates,...choice]),[1]);
 await assert.rejects(resolve([...candidates,...choice,'--candidate','c']),/provider set changed/);
 await assert.rejects(resolve([...candidates,'--choice','c','--previous','a','--previous','b']),/conflict/);
 await assert.rejects(resolve([...candidates,'--choice','a','--previous','a','--previous','a']),/invalid selection/);
 await assert.rejects(resolve([...candidates,...choice,'--resource','mesh:car.a','--candidate','a','--candidate','b']),/conflict/);
 assert.deepEqual(await resolve(['--resource','raw/asset_010.bin','--candidate','a']),[0]);
 await assert.rejects(resolve(['--resource','missing']),/conflict/);
 await assert.rejects(resolve(['--choice','a']),/Invalid/);
});
test('composition uses compiled winners even if the UI conflict summary is bypassed',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-provider-service-'));
 try{
  const service=new LauncherService({root:path.join(root,'profile'),bin,config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();service.state.disc={region:'PAL'};
  for(const [id,color] of [['a','1 0 0'],['b','0 1 0']]){
   const source=path.join(root,id);await fs.mkdir(source);
   await fs.writeFile(path.join(source,'mod.toml'),'[materials]\n"car.a"="lit opaque 0.5 0 '+color+' 1 0 0 0"\n');
   const mod=await service.importMod(source);await service.toggleMod(mod.id,true);
  }
  service.conflicts=()=>[];
  await assert.rejects(service.composeMods(),/conflict/);
  assert.equal((await fs.readdir(service.root)).filter(n=>n.startsWith('active-mods-')).length,0);
  const [a,b]=service.state.mods;await service.resolveConflict('material:car.a',b.id);
  service.state.resolutions['material:car.a'].candidates.reverse();
  assert.equal(service.overlaps()[0].winner,b.id);
  const output=await service.composeMods();
  const manifest=JSON.parse(await run(tool,[path.join(output,'mod.toml')]));
  assert.equal(manifest.materials['car.a'],b.manifest.materials['car.a']);
  service.state.resolutions['material:car.a'].candidates=[a.id,'outdated-provider'];
  await assert.rejects(service.composeMods(),/provider set changed/);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('large native provider batches use bounded stdin rather than command-line limits',async()=>{
 const tokens=[];
 for(let i=0;i<600;i++)tokens.push('--resource','texture:car.material.'+i,
  '--candidate','provider-a','--candidate','provider-b','--choice','provider-b',
  '--previous','provider-b','--previous','provider-a');
 const input=Buffer.from(tokens.join('\0')+'\0');assert.ok(input.length>32767);
 const output=JSON.parse(await run(tool,['--resolve-providers-stdin'],{input}));
 assert.deepEqual(output,Array(600).fill(1));
 await assert.rejects(run(tool,['--resolve-providers-stdin'],{input:Buffer.from('--resource\0unterminated')}),/Invalid/);
 await assert.rejects(run(tool,['--resolve-providers-stdin'],{input:Buffer.alloc(8*1024*1024+1)}),/Invalid/);
 assert.deepEqual(JSON.parse(await run(tool,['--resolve-providers-stdin'],{input:Buffer.alloc(0)})),[]);
});
