const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
function dialogs(){
 const source=fs.readFileSync(path.join(__dirname,'../renderer/app.js'),'utf8');
 const dialog={innerHTML:'',classList:{add(){}},addEventListener(){},showModal(){}};
 const context={project:null,state:{mods:[{id:'sample',manifest:{materials:{'car.0.material.0':'lit blend 0.35 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9'}}}]},carParts:[{bank:10,part:0,rival:false,name:'Example'}],$:()=>dialog,esc:String,displayedShiftSpeed:x=>x/10};
 vm.createContext(context);vm.runInContext(source.slice(source.indexOf('function materialDialog(')),context);return {context,dialog};
}
test('material hierarchy preserves every control and its storage index',()=>{
 const {context,dialog}=dialogs();context.materialDialog('sample','car.0.material.0');
 const [basic,advanced]=dialog.innerHTML.split('<details');
 for(const i of [2,3,4,5,6,7])assert.ok(basic.includes(`data-property="${i}"`));
 for(const i of [0,1,8,9,10])assert.ok(!basic.includes(`data-property="${i}"`)&&advanced.includes(`data-property="${i}"`));
 const controls=[...dialog.innerHTML.matchAll(/data-property="(\d+)"/g)].map(m=>Number(m[1]));
 assert.deepEqual(controls.sort((a,b)=>a-b),Array.from({length:11},(_,i)=>i));
 assert.match(dialog.innerHTML,/Opacity: 0 = invisible, 1 = solid/);
 assert.match(dialog.innerHTML,/does not create a headlight beam/);
});
test('part material form works without a library package and carries the project revision',()=>{
 const {context,dialog}=dialogs();context.project={revision:9,changes:[{kind:'car',id:'car.0.material.0',field:'material',value:'lit opaque 0.7 0 1 1 1 1 0 0 0'}]};
 context.materialDialog('','car.0.material.0',{materials:{'car.0.material.0':'lit opaque 0.35 0 1 1 1 1 0 0 0'}});
 assert.match(dialog.innerHTML,/data-revision="9"/);assert.match(dialog.innerHTML,/Technical identity/);
 assert.match(dialog.innerHTML,/id="property-2"[^>]*value="0.7"/);
 assert.match(dialog.innerHTML,/id="material-key"[^>]*readonly/);
 assert.equal([...dialog.innerHTML.matchAll(/data-property="/g)].length,11);
});
test('basic transmission is visible and advanced engine and shift values remain editable',()=>{
 const {context,dialog}=dialogs();
 const keys=['manualOnly','topGear','revLimit','redline','automaticAccelerationScale',...Array.from({length:6},(_,i)=>['downshift'+(i+1),'upshift'+(i+1)]).flat()];
 context.carSpecDialog(10,{fields:keys.map(key=>({key,value:key==='manualOnly'?0:100,min:0,max:10000}))});
 const [basic,advanced]=dialog.innerHTML.split('<details');
 for(const key of ['manualOnly','topGear'])assert.ok(basic.includes(`data-spec="${key}"`));
 for(const key of keys.slice(2))assert.ok(!basic.includes(`data-spec="${key}"`)&&advanced.includes(`data-spec="${key}"`));
 assert.equal([...dialog.innerHTML.matchAll(/data-spec="/g)].length,keys.length);
 assert.match(dialog.innerHTML,/data-speed="true"/);
 assert.match(dialog.innerHTML,/Values for unused gears are kept/);
 context.carSpecDialog(10,{revision:7,fields:[...keys,'price','upgradePrice','unlockClass'].map(key=>({key,value:0,min:0,max:10000}))});
 const basicWithShop=dialog.innerHTML.split('<details')[0];
 for(const key of ['price','upgradePrice','unlockClass'])assert.ok(basicWithShop.includes(`data-spec="${key}"`));
 assert.match(dialog.innerHTML,/data-revision="7"/);assert.match(dialog.innerHTML,/Save & apply/);
});

test('all handling scalars are editable once and grouped separately from basic purchasing',()=>{
 const {context,dialog}=dialogs();const keys=['steeringGripResponse','steerResponse','referenceTurnRadius','negconSteeringAssistScale','speedDragDivisor','baseSteeringGrip'];
 const required=['manualOnly','topGear',...Array.from({length:6},(_,i)=>['downshift'+(i+1),'upshift'+(i+1)]).flat()];
 context.carSpecDialog(10,{revision:4,fields:[...required,...keys].map(key=>({key,value:100,min:0,max:32767}))});
 const basic=dialog.innerHTML.split('<details')[0];for(const key of keys){assert.ok(!basic.includes('data-spec="'+key+'"'));assert.equal(dialog.innerHTML.split('data-spec="'+key+'"').length-1,1);}
 assert.match(dialog.innerHTML,/Advanced handling &amp; drag|Advanced handling & drag/);assert.match(dialog.innerHTML,/Larger values reduce drag/);
});

test('engine arrays expose every original index without duplicate controls',()=>{
 const {context,dialog}=dialogs();const arrays={torqueCurve:16,torqueBand:16,torqueLossValue:10,torqueLossRpm:9,gearLoad:6,gearRatio:7,torqueScale:6};
 const keys=['manualOnly',...Array.from({length:6},(_,i)=>['downshift'+(i+1),'upshift'+(i+1)]).flat(),...Object.entries(arrays).flatMap(([key,count])=>Array.from({length:count},(_,i)=>key+i))];
 context.carSpecDialog(10,{fields:keys.map(key=>({key,value:100,min:0,max:10000}))});
 for(const [key,count]of Object.entries(arrays))for(let i=0;i<count;i++)assert.equal(dialog.innerHTML.split('data-spec="'+key+i+'"').length-1,1);
 assert.match(dialog.innerHTML,/Advanced engine curves & gearing/);assert.match(dialog.innerHTML,/not a conventional ratio display/);
});
test('car identity is visible in the basic form and preserves text values',()=>{
 const {context,dialog}=dialogs();const keys=['manualOnly',...Array.from({length:6},(_,i)=>['downshift'+(i+1),'upshift'+(i+1)]).flat()];
 context.carSpecDialog(10,{fields:[...keys.map(key=>({key,value:100,min:0,max:10000})),...['name','manufacturer','className'].map(key=>({key,type:'text',value:'Coast Special',maxLength:31}))]});
 const basic=dialog.innerHTML.split('<details')[0];for(const key of ['name','manufacturer','className'])assert.match(basic,new RegExp('data-spec="'+key+'" data-text'));
 assert.equal([...dialog.innerHTML.matchAll(/data-text/g)].length,3);assert.match(basic,/type="text" maxlength="31" value="Coast Special"/);
});
