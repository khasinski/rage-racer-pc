const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),os=require('node:os'),path=require('node:path');
const {createHash}=require('node:crypto');
const {run}=require('../main/service.cjs');
const cli=path.resolve(__dirname,'../resources/bin','rage-mod-cli'+(process.platform==='win32'?'.exe':''));
const hash=bytes=>createHash('sha256').update(bytes).digest('hex');

test('native SHA-256 matches Node across block and streaming boundaries',async()=>{
  const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-hash-'));
  try{
    const file=path.join(root,'bytes');
    for(const size of [0,1,55,56,63,64,65,127,65535,65536,65537]){
      const bytes=Buffer.alloc(size);for(let i=0;i<size;i++)bytes[i]=(i*31+17)%256;
      await fs.writeFile(file,bytes);
      assert.equal(JSON.parse(await run(cli,['--hash-file',file])).sha256,hash(bytes),`length ${size}`);
    }
    await fs.mkdir(path.join(root,'textures'));
    const bytes=await fs.readFile(file);await fs.copyFile(file,path.join(root,'textures/check.png'));
    const batch=names=>run(cli,['--hash-files-stdin',root],{input:Buffer.from(JSON.stringify(names))});
    assert.deepEqual(JSON.parse(await batch(['textures/check.png'])),[['textures/check.png',hash(bytes)]]);
    assert.deepEqual(JSON.parse(await batch([])),[]);
    for(const names of [null,['../bytes'],['textures/missing.png'],['textures/check.png','textures/check.png'],[42]])
      await assert.rejects(batch(names),/invalid inventory/);
    await fs.truncate(path.join(root,'textures/check.png'),128*1024*1024+1);
    await assert.rejects(batch(['textures/check.png']),/size limit/);
  }finally{await fs.rm(root,{recursive:true,force:true});}
});

test('native active build rejects ambiguous identity and damaged or invalid profiles',async()=>{
  const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-active-Łódź-東京-'));
  try{
    const id='12345678-1234-1234-1234-123456789abc',dir=path.join(root,'builds',id);
    await fs.mkdir(dir,{recursive:true});
    const profile=await fs.readFile(path.resolve(__dirname,'../../cars.toml'));
    const manifest='[mod]\nid = "editor-project"\n';
    await fs.writeFile(path.join(dir,'cars.toml'),profile);await fs.writeFile(path.join(dir,'mod.toml'),manifest);
    const source={region:'PAL',fingerprint:'sha256:'+hash('source')};
    const pointer={format:1,build:id,source,profileHash:hash(profile),manifestHash:hash(manifest)};
    const file=path.join(root,'active.json');
    const read=()=>run(cli,['--active-build',root,source.region,source.fingerprint]);
    assert.deepEqual(JSON.parse(await read()),{active:false});
    await fs.writeFile(file,JSON.stringify(pointer));
    const result=JSON.parse(await read());assert.equal(result.active,true);
    assert.equal(path.normalize(result.directory),dir);assert.equal(path.normalize(result.profile),path.join(dir,'cars.toml'));
    for(const bad of [
      JSON.stringify(pointer).replace('"format":1','"format":1,"format":1'),
      JSON.stringify({...pointer,format:1.5}),
      JSON.stringify({...pointer,build:'../'+id}),
      JSON.stringify({...pointer,source:{...source,region:'NTSC-J'}}),
      JSON.stringify({...pointer,source:{...source,fingerprint:'sha256:'+hash('other')}}),
      JSON.stringify(pointer).replace('"region":"PAL"','"region":"PAL","region":"PAL"'),
    ]){await fs.writeFile(file,bad);await assert.rejects(read());}
    await fs.writeFile(file,JSON.stringify(pointer));
    await fs.appendFile(path.join(dir,'mod.toml'),'# changed');await assert.rejects(read(),/manifest.*damaged/);
    await fs.writeFile(path.join(dir,'mod.toml'),manifest);
    await fs.appendFile(path.join(dir,'cars.toml'),'# changed');await assert.rejects(read(),/profile.*damaged/);
    const invalid='[[cars]]\nid = "car_00_g0"\nmodel = 0\ngrade = 0\nprice = -1\n';
    await fs.writeFile(path.join(dir,'cars.toml'),invalid);
    await fs.writeFile(file,JSON.stringify({...pointer,profileHash:hash(invalid)}));
    await assert.rejects(read(),/runtime validation/);
    await fs.writeFile(path.join(dir,'cars.toml'),profile);
    await fs.mkdir(path.join(dir,'textures'));
    const payloadFile=path.join(dir,'textures/light.png');await fs.writeFile(payloadFile,'payload fixture');
    const digest=hash('payload fixture');
    const activate=async entries=>{
      const bytes=JSON.stringify(entries);await fs.writeFile(path.join(dir,'files.json'),bytes);
      await fs.writeFile(file,JSON.stringify({...pointer,format:2,payloadHash:hash(bytes)}));
    };
    await activate([['textures/light.png',digest]]);assert.equal(JSON.parse(await read()).active,true);
    await fs.appendFile(payloadFile,'changed');await assert.rejects(read(),/payload.*damaged/);
    await fs.writeFile(payloadFile,'payload fixture');
    for(const entries of [
      [['../outside.png',digest]],
      [['textures/light.png',digest],['textures/light.png',digest]],
      [['textures/light.png',digest],['raw/asset_000.bin',digest]],
      [['cars.toml',pointer.profileHash]],
      [['textures/light.png','invalid']],
      [['textures/missing.png',digest]],
    ]){await activate(entries);await assert.rejects(read(),/payload.*damaged/);}
    await activate([]);assert.equal(JSON.parse(await read()).active,true);
    await fs.appendFile(path.join(dir,'files.json'),' ');await assert.rejects(read(),/payload.*damaged/);
  }finally{await fs.rm(root,{recursive:true,force:true});}
});
