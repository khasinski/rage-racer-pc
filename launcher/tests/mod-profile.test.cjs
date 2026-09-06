const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {run}=require('../main/service.cjs');
const tool=path.resolve(__dirname,'../resources/bin/rage-mod-cli'+(process.platform==='win32'?'.exe':''));
test('compiled profile writer validates before creation and never overwrites',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-profile-writer-'));
 try{
  const output=path.join(root,'mod.toml');
  const tables={textures:{'car.a':'textures/a.png'},materials:{},meshes:{}};
  const write=value=>run(tool,['--write-profile-stdin',output],{input:Buffer.from(JSON.stringify(value))});
  for(const bad of [null,{}, {...tables,extra:{}},{...tables,textures:{'car.a':'../escape.png'}},
   {...tables,materials:{'car.a':'invalid'}},{...tables,textures:{'car.a':'textures/a.png\n[mod]'}},
   {...tables,textures:{'car.a':'textures/a\0.png'}},{...tables,meshes:[]}]){
   await assert.rejects(write(bad),/Invalid mod manifest/);
   await assert.rejects(fs.access(output),{code:'ENOENT'});
  }
  await write(tables);
  const before=await fs.readFile(output);
  const parsed=JSON.parse(await run(tool,[output]));
  assert.equal(parsed.id,'launcher-profile');assert.deepEqual(parsed.textures,tables.textures);
  await assert.rejects(write(tables),/profile output failure/);
  assert.deepEqual(await fs.readFile(output),before);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
