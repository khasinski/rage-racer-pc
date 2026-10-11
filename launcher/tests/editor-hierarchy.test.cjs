const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
function dialogs(){
 const source=fs.readFileSync(path.join(__dirname,'../renderer/app.js'),'utf8');
 const dialog={innerHTML:'',classList:{add(){}},addEventListener(){},showModal(){}};
 const context={state:{mods:[{id:'sample',manifest:{materials:{'car.0.material.0':'lit blend 0.35 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9'}}}]},carParts:[{bank:10,part:0,rival:false,name:'Example'}],$:()=>dialog,esc:String,displayedShiftSpeed:x=>x/10};
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
