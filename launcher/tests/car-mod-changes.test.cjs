const {test}=require('node:test'),assert=require('node:assert/strict');
test('workshop links player and rival changes without mixing rival bodies or claiming raw previews',async()=>{
 const {carModChanges:changes}=await import('../renderer/mod-car-targets.mjs');
 const mod=(meshes={},materials={},files=[])=>({manifest:{meshes,materials},files});
 const player={bank:10,part:0,rival:false},rival={bank:88,part:5,rival:true};
 assert.deepEqual(changes(mod({'car.player.10.part.2':'wheel'}),player),{affected:true,preview:true,raw:false});
 assert.equal(changes(mod({'car.player.12.part.0':'body'}),player).affected,false);
 assert.equal(changes(mod({'car.rival.88.part.2':'wheel'}),rival).affected,false);
 assert.equal(changes(mod({'car.rival.88.part.7':'wheel'}),rival).preview,true);
 assert.equal(changes(mod({}, {'track.big1.model-bank-1.material.0':'paint'}),rival).preview,true);
 assert.deepEqual(changes(mod({}, {},['raw/asset_011.bin']),player),{affected:true,preview:false,raw:true});
 assert.equal(changes(mod({}, {},['raw/asset_013.bin']),player).affected,false);
 assert.equal(changes(mod({}, {},['raw/asset_089.bin']),rival).affected,false);
});
