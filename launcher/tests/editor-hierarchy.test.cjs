const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
function inspector(fields){
 const source=fs.readFileSync(path.join(__dirname,'../renderer/car-studio.js'),'utf8'),start=source.indexOf('function renderAccordionInspector('),end=source.indexOf('function openLoadCarModal(',start);
 const context={openAccordions:new Set(['identity','engine','gearbox','handling']),esc:String,displayedShiftSpeed:v=>v/10,renderFitmentAccordion:()=>'',renderTorqueChart:()=>'',carGradeBadge:()=>''};vm.createContext(context);vm.runInContext(source.slice(start,end),context);return context.renderAccordionInspector({bank:10,part:0,rival:false},[],{revision:7,fields});
}
test('material editor keeps all eleven controls and separates lighting from basic finish',async()=>{
 const {materialForm}=await import('../renderer/material-editor.mjs'),html=materialForm('car.0.material.0','lit opaque 0.7 0.2 1 1 1 1 0 0 0',9),[basic,advanced]=html.split('<details>');for(const i of [2,3,4,5,6,7])assert.ok(basic.includes(`data-property="${i}"`));for(const i of [0,1,8,9,10])assert.ok(advanced.includes(`data-property="${i}"`));assert.equal([...html.matchAll(/data-property="/g)].length,11);assert.match(html,/data-revision="9"/);assert.match(html,/id="property-2"[^>]*value="0.7"/);assert.match(html,/does not create a headlight beam/);
});
test('actual Car Studio inspector exposes every scalar and original array index exactly once',()=>{
 const arrays={torqueCurve:16,torqueBand:16,torqueLossValue:10,torqueLossRpm:9,gearLoad:6,gearRatio:7,torqueScale:6};const keys=['manualOnly','topGear','revLimit','redline','automaticAccelerationScale','steeringGripResponse','steerResponse','referenceTurnRadius','negconSteeringAssistScale','speedDragDivisor','baseSteeringGrip','price','upgradePrice','unlockClass',...Array.from({length:6},(_,i)=>['downshift'+(i+1),'upshift'+(i+1)]).flat(),...Object.entries(arrays).flatMap(([k,n])=>Array.from({length:n},(_,i)=>k+i))];const fields=keys.map(key=>({key,value:100,baseValue:50,min:0,max:10000}));fields.push(...['name','manufacturer','className'].map(key=>({key,type:'text',value:'Coast Special',maxLength:31})));const html=inspector(fields);for(const f of fields)assert.equal(html.split(`data-spec="${f.key}"`).length-1,1,f.key);assert.match(html,/data-revision="7"/);assert.match(html,/data-speed="true"/);assert.match(html,/data-text/);assert.match(html,/data-reset-field="price"/);
});
test('both studios install the working shared material editor',()=>{for(const studio of ['car','track']){const source=fs.readFileSync(path.join(__dirname,'../renderer/'+studio+'-studio.js'),'utf8');assert.match(source,/installMaterialEditor\(\{request/);}});

test('preview completion during cancellation never dereferences null or clears a newer job',async()=>{
 for(const kind of ['car','track']){
  const source=fs.readFileSync(path.join(__dirname,'../renderer/'+kind+'-studio.js'),'utf8'),start=source.indexOf('async function cancelPreview() {'),end=source.indexOf('\nfunction ',start),context={previewPromise:Promise.resolve(),previewToken:0,previewBusy:true,loadingPreviewKey:'old',loading3dKey:'old'};context.api={cancel:async scope=>{assert.equal(scope,'preview');context.previewPromise=null;}};vm.createContext(context);vm.runInContext(source.slice(start,end),context);await context.cancelPreview();assert.equal(context.previewBusy,false);
  const next=Promise.resolve();context.previewPromise=Promise.resolve();context.previewBusy=true;context.api.cancel=async()=>{context.previewToken++;context.previewPromise=next;};await context.cancelPreview();assert.equal(context.previewPromise,next);assert.equal(context.previewBusy,true);
 }
});
