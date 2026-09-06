const {test}=require('node:test'),assert=require('node:assert/strict');
test('appearance-only mods offer catalog car bodies for affected player and rival banks',async()=>{
 const {modCarBanks,modCarTargets}=await import('../renderer/mod-car-targets.mjs');
 const mod={manifest:{materials:{'car.1.material.0':'value'},textures:{'track.oval6.model-bank-1.material.4.variant.0':'image','track.big1.terrain.material.0':'terrain'}}};
 assert.deepEqual([...modCarBanks(mod)],['player:12','rival:134']);
 const parts=[{bank:12,part:0,rival:false},{bank:12,part:2,rival:false},{bank:10,part:0,rival:false},{bank:134,part:0,rival:true},{bank:134,part:5,rival:true},{bank:134,part:2,rival:true}];
 assert.deepEqual(modCarTargets(mod,parts),[parts[0],parts[3],parts[4]]);
 assert.equal(modCarBanks({manifest:{materials:{'car.99.material.0':'bad'}}}).size,0);
});
