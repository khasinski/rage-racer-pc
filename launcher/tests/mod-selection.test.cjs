const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {run,LauncherService}=require('../main/service.cjs');
const bin=path.resolve(__dirname,'../resources/bin');
const tool=path.join(bin,'rage-mod-cli'+(process.platform==='win32'?'.exe':''));
const mod=(id,version='1',region='PAL',manifest='')=>['--mod',id,version,region,manifest];
const requires=(id,version='')=>['--requires',id,version];
const order=async args=>JSON.parse(await run(tool,['--check-selection',...args]));

test('selection stdin supports large graphs and rejects malformed framing',async()=>{
 const streamed=input=>run(tool,['--check-selection-stdin'],{input});
 assert.deepEqual(JSON.parse(await streamed(Buffer.alloc(0))),[]);
 const args=[];
 const id=i=>'package-'+i+'x'.repeat(110);
 for(let i=0;i<128;i++){
  args.push(...mod(id(i)));
  if(i<127)args.push(...requires(id(i+1)));
 }
 const input=Buffer.from(args.join('\0')+'\0');
 assert.ok(input.length>32767);
 assert.deepEqual(JSON.parse(await streamed(input)),Array.from({length:128},(_,i)=>127-i));
 await assert.rejects(streamed(Buffer.from('--mod')),/Invalid/);
 await assert.rejects(streamed(Buffer.from('--mod\0')),/Invalid/);
 await assert.rejects(streamed(Buffer.alloc(8*1024*1024+1)),/Invalid/);
 await assert.rejects(streamed(Buffer.alloc(262145)),/Invalid/);
});

test('native selection handles exact versions, regions, ambiguity, cycles and bounds',async()=>{
 assert.deepEqual(await order([]),[]);
 assert.deepEqual(await order([...mod('addon'),...requires('base','1'),...mod('base')]),[1,0]);
 assert.deepEqual(await order([...mod('addon'),...requires('base','1'),...mod('base','2'),...mod('base')]),[2,0,1]);
 await assert.rejects(order([...mod('addon'),...requires('base'),...mod('base','2'),...mod('base')]),/multiple matching/);
 await assert.rejects(order([...mod('addon'),...requires('base','2'),...mod('base')]),/requires base version 2/);
 await assert.rejects(order([...mod('addon'),...requires('base'),...mod('base','1','NTSC-U')]),/missing matching/);
 await assert.rejects(order([...mod('a'),...requires('b'),...mod('b'),...requires('a')]),/cycle/);
 await assert.rejects(order([...mod('a'),...requires('a')]),/cycle/);
 await assert.rejects(order(['--requires','a','']),/Invalid/);
 await assert.rejects(order(['--mod','a']),/Invalid/);
 const chain=[];for(let i=0;i<128;i++){chain.push(...mod('m'+i));if(i<127)chain.push(...requires('m'+(i+1)));}
 assert.deepEqual(await order(chain),Array.from({length:128},(_,i)=>127-i));
 await assert.rejects(order([...chain,...mod('overflow')]),/Invalid/);
 const tooMany=mod('a');for(let i=0;i<49;i++)tooMany.push(...requires('b'));
 await assert.rejects(order([...tooMany,...mod('b')]),/Invalid/);
});

test('TOML and package dependencies share a graph without conflating identities',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-selection-'));
 try{
  const addon=path.join(root,'addon.toml'),base=path.join(root,'base.toml');
  await fs.writeFile(addon,'[mod]\nid="addon-mesh"\nrequires=["base-mesh"]');
  await fs.writeFile(base,'[mod]\nid="base-mesh"');
  const args=[...mod('addon-package','1','PAL',addon),...mod('base-package','1','PAL',base)];
  assert.deepEqual(await order(args),[1,0]);
  await assert.rejects(order([...mod('addon-package','1','PAL',addon),...mod('base-mesh')]),/missing matching/);
  await assert.rejects(order([...args,...mod('copy','1','PAL',base)]),/multiple matching/);
  await assert.rejects(order([...args,...requires('addon-package')]),/cycle/);
  await fs.writeFile(base,'[mod]\nid="base-mesh"\nrequires=["addon-mesh"]');
  await assert.rejects(order(args),/cycle/);
  await fs.writeFile(base,'[mod]\nschema_version=2');
  await assert.rejects(order(args),/Invalid/);
  await assert.rejects(order(mod('a','1','PAL',path.join(root,'missing'))),/Invalid/);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('launcher validates installed TOML at activation and again before composition',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-selection-service-'));
 try{
  const service=new LauncherService({root:path.join(root,'profile'),bin,config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();service.state.disc={region:'PAL'};
  async function install(id,text){const folder=path.join(root,id);await fs.mkdir(folder);await fs.writeFile(path.join(folder,'mod.toml'),text);return service.importMod(folder);}
  const addon=await install('addon','[mod]\nid="addon"\nrequires=["base"]');
  await assert.rejects(service.toggleMod(addon.id,true),/requires base/);assert.equal(addon.enabled,false);
  const base=await install('base','[mod]\nid="base"');
  await service.toggleMod(base.id,true);await service.toggleMod(addon.id,true);
  assert.deepEqual((await service.validateModSelection()).map(m=>m.id),[base.id,addon.id]);
  await assert.rejects(service.toggleMod(base.id,false),/requires base/);assert.equal(base.enabled,true);
  await assert.rejects(service.removeMod(base.id),/requires base/);assert.ok(service.state.mods.includes(base));
  const composed=await service.composeMods();await fs.access(path.join(composed,'mod.toml'));
  await fs.writeFile(path.join(service.root,'mods',base.id,'mod.toml'),'[mod]\nid="base"\nrequires=["addon"]');
  await assert.rejects(service.composeMods(),/cycle/);
  await fs.writeFile(path.join(service.root,'mods',base.id,'mod.toml'),'[mod]\nid="renamed"');
  await assert.rejects(service.composeMods(),/requires base/);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
