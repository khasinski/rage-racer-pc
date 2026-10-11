const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs/promises'),os=require('node:os'),path=require('node:path');
const {LauncherService}=require('../main/service.cjs');

test('original track catalog preserves every class, direction and named block without booting the game',async()=>{
  const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-track-catalog-'));
  try {
    await fs.mkdir(path.join(root,'raw'));
    const bytes=Buffer.alloc(88);
    for(let i=0;i<11;i++)bytes.writeUInt32LE(44+i*4,i*4);
    for(let bank=88;bank<=134;bank+=2)await fs.writeFile(path.join(root,'raw',`asset_${String(bank).padStart(3,"0")}.bin`),bytes);
    const service=new LauncherService({root,bin:path.resolve(__dirname,'../../build/release'),config:''});
    service.state.disc={data:root,region:'pal'};
    service.requireReady=()=>{};
    const {tracks}=await service.tracks();
    assert.equal(tracks.length,24);
    assert.equal(new Set(tracks.map(t=>t.id)).size,24);
    assert.equal(tracks.filter(t=>t.available).length,22);
    assert.deepEqual(tracks[0].directions,['forward','reverse']);
    assert.equal(tracks[0].name,'Mythical Coast');
    assert.equal(tracks[0].blocks.length,11);
    assert.equal(tracks[0].blocks[4].role,'road');
    assert.equal(tracks[0].blocks[10].role,'cameras');
    // A broken source cannot yield a partially successful catalog.
    await fs.writeFile(path.join(root,'raw','asset_134.bin'),Buffer.alloc(44));
    await assert.rejects(service.tracks(),/Invalid track pack/);
  } finally {await fs.rm(root,{recursive:true,force:true});}
});
