const {test}=require('node:test');const assert=require('node:assert/strict');
const {defaults,validate,patchIni}=require('../main/config.cjs');
test('settings reject unknown options, injected INI lines and out-of-range values',()=>{
 assert.deepEqual(validate(defaults),defaults);
 for(const input of [{'content.car_names':'japanese'},{'video.internal_scale':'NaN'},{'video.internal_scale':'17'},{'video.aspect':'16:9\n[tools]'},{'input.analog':'false'}])assert.throws(()=>validate(input));
});
test('configuration retains unrelated settings and updates duplicate known keys',()=>{
 const source='# keep this\n[video]\ninternal_scale=2\nunknown=retain\n[video]\ninternal_scale=3\n[custom]\nthing=yes\n';
 const result=patchIni(source,{'video.internal_scale':'4','hud.anchor':'edges'});
 assert.equal((result.match(/internal_scale = 4/g)||[]).length,2);
 assert.match(result,/# keep this/);assert.match(result,/unknown=retain/);assert.match(result,/thing=yes/);assert.match(result,/\[hud\]\nanchor = edges/);
 assert.throws(()=>patchIni('',{'disc.image':'a\nb'}));
});
