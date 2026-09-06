const {test}=require('node:test'),assert=require('node:assert/strict'),path=require('node:path');
const display=require('../main/car-names.cjs'),{run}=require('../main/service.cjs');
test('workshop car names follow disc region using compiled name tables',async()=>{
 const tool=path.resolve(__dirname,'../resources/bin/rage-save-cli'+(process.platform==='win32'?'.exe':''));
 const names=JSON.parse(await run(tool,['car-names']));
 assert.equal(names.international.length,13);assert.equal(names.japanese.length,13);
 for(let i=0;i<13;i++){
  const source=names.international[i];
  assert.equal(display(source+' wheel 2','NTSC-J',names),names.japanese[i]+' wheel 2');
  for(const region of ['PAL','NTSC-U'])assert.equal(display(source,region,names),source);
 }
 assert.equal(display('Erriso','NTSC-J',names),'Alouette');
 assert.equal(display('Squaldon','NTSC-J',names),'Dragone');
 assert.equal(display('Unknown model','NTSC-J',names),'Unknown model');
});
