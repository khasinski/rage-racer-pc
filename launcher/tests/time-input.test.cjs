const {test}=require('node:test'),assert=require('node:assert/strict');
test('full time paste preserves milliseconds and rejects overflow or ambiguous syntax',async()=>{
 const {parseTimeClipboard:parse}=await import('../renderer/time-input.mjs');
 assert.deepEqual(parse('01:40.765'),{m:1,s:40,ms:765});
 assert.deepEqual(parse(' 00 : 00 . 001 '),{m:0,s:0,ms:1});
 assert.deepEqual(parse('35791:23.647'),{m:35791,s:23,ms:647});
 for(const invalid of ['35791:23.648','01:60.000','-01:40.765','01:40.76','01:40.7650','1e2:00.000','40','01:40.765 trailing'])assert.equal(parse(invalid),null,invalid);
});
