const {test}=require('node:test'),assert=require('node:assert/strict');
const path=require('node:path');
const {run}=require('../main/service.cjs');
const tool=path.resolve(__dirname,'../resources/bin/rage-mod-cli'+(process.platform==='win32'?'.exe':''));
const check=value=>run(tool,['--check-files-stdin'],{input:Buffer.from(JSON.stringify(value))});
test('compiled directory inventory bounds names, roots and depth',async()=>{
 const dirs=value=>run(tool,['--check-directories-stdin'],{input:Buffer.from(JSON.stringify(value))});
 await dirs(['raw','textures/a/b/c/d/e/f/g','meshes/body']);await dirs([]);
 for(const value of [['other'],['textures/a b'],['textures/../raw'],['textures/a/b/c/d/e/f/g/h'],
  ['textures\0hidden'],[42],{},Array(10001).fill('raw')])await assert.rejects(dirs(value),/mod file/);
});
test('launcher inventory validates empty directories through the native policy',async()=>{
 const fs=require('node:fs/promises'),os=require('node:os');
 const {inventory}=require('../main/mods.cjs');
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-directory-policy-'));
 try{
  await fs.writeFile(path.join(root,'mod.toml'),'[mod]');
  await fs.mkdir(path.join(root,'textures','valid'),{recursive:true});
  assert.deepEqual(await inventory(root,tool),['mod.toml']);
  await fs.mkdir(path.join(root,'textures','invalid folder'));
  await assert.rejects(inventory(root,tool),/mod file/);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
test('compiled inventory validates complete batches without silently truncating paths',async()=>{
 await check(['mod.toml','rage-mod.json','raw/asset_134.bin','textures/a.png','meshes/a.rmesh']);
 await check([]);
 for(const files of [['raw/asset_135.bin'],['textures/../a.png'],['textures/a.png\0.exe'],[42],{},
  Array(10001).fill('mod.toml'),['run.js'],['textures/folder/a b.png']])
  await assert.rejects(check(files),/mod file/);
 await assert.rejects(run(tool,['--check-files-stdin'],{input:Buffer.from('[')}),/mod file/);
});
test('compiled copy roles separate overrides, backing files and package metadata',async()=>{
 const roles=async value=>JSON.parse(await run(tool,['--file-dispositions-stdin'],{input:Buffer.from(JSON.stringify(value))}));
 assert.deepEqual(await roles([['raw/asset_010.bin',0],['textures/a.png',0],['textures/a.png',2],
  ['textures/a.png',4],['textures/a.png',6],['meshes/a.rmesh',2],['meshes/unused.rmesh',0],
  ['mod.toml',0],['rage-mod.json',0],['manifest.json',0]]),[1,1,2,4,6,2,0,0,0,0]);
 for(const value of [[['meshes/a.rmesh',4]],[['raw/asset_000.bin',2]],[['textures/a.png',1]],
  [['textures/a.png',-1]],[['textures/a.png',2.5]],[['textures/a.png',4294967296]],[['mod.toml']]])
  await assert.rejects(roles(value),/mod file/);
});
