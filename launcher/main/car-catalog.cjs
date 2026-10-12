const path=require('node:path');
module.exports=async function carCatalog(service,signal){
 const project=await service.loadProject(),{run}=require('./service.cjs');
 const catalog=JSON.parse(await service.carTool(['--set','tools.car_catalog=true'],signal));
 const names=JSON.parse(await run(service.tool('rage-save-cli'),['car-names'],{signal}));
 const profile=JSON.parse(await run(service.tool('rage-mod-cli'),['--car-profile',path.join(service.bin,service.state.disc.region==='NTSC-J'?'cars.ntscj.toml':'cars.toml')],{signal}));
 const authoredNames=new Map((project?.snapshot().changes||[]).filter(c=>c.kind==='car'&&c.field==='name').map(c=>[c.id,c.value]));
 const display=require('./car-names.cjs');
 return {...catalog,parts:catalog.parts.map(part=>{
  const car=!part.rival&&part.part===0?profile.cars.find(c=>c.variant===(part.bank-10)/2):null;
  return {...part,name:car&&authoredNames.has(car.id)?authoredNames.get(car.id):display(part.name,service.state.disc.region,names)};
 })};
};
