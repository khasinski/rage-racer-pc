const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {run}=require('../main/service.cjs');
const tool=path.resolve(__dirname,'../resources/bin/rage-mod-cli'+(process.platform==='win32'?'.exe':''));
test('compiled legacy index writer validates all entries before creating output',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-legacy-write-'));
 try{
  const output=path.join(root,'index.txt');
  const write=value=>run(tool,['--write-legacy-index-stdin',output],{input:Buffer.from(JSON.stringify(value))});
  for(const bad of [null,{},[[135,'a.json']],[[0,'../a.json']],[[0,'a.json\n1 b.json']],
   [[0,'a.json\0']],[[0,'a.json'],[1,'bad']],[[0,'a.json',1]],[['0','a.json']]]){
   await assert.rejects(write(bad),/legacy texture index/);
   await assert.rejects(fs.access(output),{code:'ENOENT'});
  }
  await write([[134,'nested/a.json'],[0,'b.json'],[134,'c.json']]);
  const expected='134 nested/a.json\n0 b.json\n134 c.json\n';
  assert.equal(await fs.readFile(output,'utf8'),expected);
  assert.deepEqual(JSON.parse(await run(tool,['--legacy-index',output])).map(e=>e.asset),[134,0,134]);
  await assert.rejects(write([]),/output failure/);
  assert.equal(await fs.readFile(output,'utf8'),expected);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
test('shared legacy texture parser preserves order and rejects unsafe or oversized lines',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-legacy-index-'));
 try{
  const file=path.join(root,'index.txt');
  async function parse(text){await fs.writeFile(file,text);return JSON.parse(await run(tool,['--legacy-index',file]));}
  assert.deepEqual(await parse('# comment\r\n134 nested/a.json\r\n0 b.json\n134 nested/a.json'),[
   {asset:134,json:'textures/nested/a.json',png:'textures/nested/a.png',resourceClaim:'legacy-textures:asset-134'},
   {asset:0,json:'textures/b.json',png:'textures/b.png',resourceClaim:'legacy-textures:asset-0'},
   {asset:134,json:'textures/nested/a.json',png:'textures/nested/a.png',resourceClaim:'legacy-textures:asset-134'}]);
  for(const text of ['135 a.json','0 ../a.json','0 a.json extra','0 a.json\0hidden',
   '0 a.json\n'+'x'.repeat(512),' '.repeat(2*1024*1024+1)])
   await assert.rejects(parse(text),/legacy texture index/);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
