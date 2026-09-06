const {test}=require('node:test'),assert=require('node:assert/strict');
test('speedometer display conversion preserves every stored shift threshold',async()=>{
 const {displayedShiftSpeed,storedShiftSpeed}=await import('../renderer/car-speed.mjs');
 assert.equal(displayedShiftSpeed(1168),160);assert.equal(storedShiftSpeed(160),1168);
 for(let raw=-32768;raw<=32767;raw++)assert.equal(storedShiftSpeed(displayedShiftSpeed(raw)),raw);
});
