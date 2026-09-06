const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {run}=require('../main/service.cjs');
const tool=path.resolve(__dirname,'../resources/bin/rage-mod-cli'+(process.platform==='win32'?'.exe':''));
test('shared legacy texture parser preserves order and rejects unsafe or oversized lines',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-legacy-index-'));
 try{
  const file=path.join(root,'index.txt');
  async function parse(text){await fs.writeFile(file,text);return JSON.parse(await run(tool,['--legacy-index',file]));}
  assert.deepEqual(await parse('# comment\r\n134 nested/a.json\r\n0 b.json\n134 nested/a.json'),[
   {asset:134,json:'textures/nested/a.json',png:'textures/nested/a.png'},
   {asset:0,json:'textures/b.json',png:'textures/b.png'},
   {asset:134,json:'textures/nested/a.json',png:'textures/nested/a.png'}]);
  for(const text of ['135 a.json','0 ../a.json','0 a.json extra','0 a.json\0hidden',
   '0 a.json\n'+'x'.repeat(512),' '.repeat(2*1024*1024+1)])
   await assert.rejects(parse(text),/legacy texture index/);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
