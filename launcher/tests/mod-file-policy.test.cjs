const {test}=require('node:test'),assert=require('node:assert/strict');
const path=require('node:path');
const {run}=require('../main/service.cjs');
const tool=path.resolve(__dirname,'../resources/bin/rage-mod-cli'+(process.platform==='win32'?'.exe':''));
const check=value=>run(tool,['--check-files-stdin'],{input:Buffer.from(JSON.stringify(value))});
test('compiled inventory validates complete batches without silently truncating paths',async()=>{
 await check(['mod.toml','rage-mod.json','raw/asset_134.bin','textures/a.png','meshes/a.rmesh']);
 await check([]);
 for(const files of [['raw/asset_135.bin'],['textures/../a.png'],['textures/a.png\0.exe'],[42],{},
  Array(10001).fill('mod.toml'),['run.js'],['textures/folder/a b.png']])
  await assert.rejects(check(files),/mod file/);
 await assert.rejects(run(tool,['--check-files-stdin'],{input:Buffer.from('[')}),/mod file/);
});
