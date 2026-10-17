const path=require('node:path');
module.exports=async function carCatalog(service,signal){
 const project=await service.loadProject(),{run}=require('./service.cjs');
 const catalog=JSON.parse(await service.carTool(['--set','tools.car_catalog=true'],signal));
 const names=JSON.parse(await run(service.tool('rage-save-cli'),['car-names'],{signal}));
 const profile=JSON.parse(await run(service.tool('rage-mod-cli'),['--car-profile',path.join(service.bin,service.state.disc.region==='NTSC-J'?'cars.ntscj.toml':'cars.toml')],{signal}));
 catalog.authored=catalog.parts.length>0;
 if(!catalog.authored){
  const roles=['Body','Body at a distance','Front wheels','Rear wheels','Wheels at a distance'];
  for(const car of profile.cars){const bank=10+car.variant*2;for(let part=0;part<22;part++)catalog.parts.push({key:`car.player.${bank}.part.${part}`,bank,part,rival:false,name:car.name+(part?' · '+(roles[part]||'Wheel phase '+part):''),original:true});}
  for(let bank=88;bank<=134;bank+=2){const raceClass=Math.floor((bank-88)/8)+1,count=raceClass===6?4:7;for(let rival=0;rival<count;rival++)for(let offset=0;offset<5;offset++){const part=rival*5+offset;catalog.parts.push({key:`car.rival.${bank}.part.${part}`,bank,part,rival:true,name:`Rival ${rival+1} · Class ${raceClass}`+(offset?' · '+roles[offset]:''),original:true});}}
 }
 const changes=project?.snapshot().changes||[];
 const authoredNames=new Map(changes.filter(c=>c.kind==='car'&&c.field==='name').map(c=>[c.id,c.value]));
 const display=require('./car-names.cjs');
 const baseParts=catalog.parts.map(part=>{
  const car=!part.rival&&part.part===0?profile.cars.find(c=>c.variant===(part.bank-10)/2):null;
  return {...part,name:car&&authoredNames.has(car.id)?authoredNames.get(car.id):display(part.name,service.state.disc.region,names)};
 });
 const customBanks=new Map();
 for(const c of changes){
  const m=c.kind==='car'&&/^custom_car_(\d+)$/.exec(c.id);
  if(m){
   const bank=Number(m[1]);
   if(bank>=200&&bank%2===0&&!customBanks.has(bank))customBanks.set(bank,{donorBank:10,name:`Custom Car #${32+(bank-200)/2}`});
   if(customBanks.has(bank)){
    if(c.field==='donor_bank')customBanks.get(bank).donorBank=Number(c.value)||10;
    if(c.field==='name'&&typeof c.value==='string')customBanks.get(bank).name=c.value;
   }
  }
 }
 for(const [bank,info] of [...customBanks.entries()].sort((a,b)=>a[0]-b[0])){
  for(let p=0;p<5;p++)baseParts.push({key:`car.player.${bank}.part.${p}`,bank,part:p,rival:false,customSlot:true,donorBank:info.donorBank,name:info.name});
 }
 return {...catalog,parts:baseParts};
};
