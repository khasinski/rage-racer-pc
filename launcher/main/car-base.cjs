const path=require('node:path');
// Preserve reconstructed profile defaults unless a selected raw bank actually
// changes that retail field. The same resolved base feeds the UI and builder.
module.exports=async function carBase(service,view,signal,profile){
 const {run}=require('./service.cjs'),{binding}=require('./car-profile.cjs');
 profile ||= path.join(service.bin,service.state.disc.region==='NTSC-J'?'cars.ntscj.toml':'cars.toml');
 const {cars}=JSON.parse(await run(service.tool('rage-mod-cli'),['--car-profile',profile],{signal}));
 const origins=new Map(),changes=new Map(),raw=require('./car-view.cjs').rawSource;
 for(const car of cars){
  const bank=10+car.variant*2,changed=new Set(),fields=new Map();
  for(const [asset,command]of [[bank+1,'--car-spec'],[bank,'--car-transmission']]){
   const relative='raw/asset_'+String(asset).padStart(3,'0')+'.bin';if(!view.files.includes(relative))continue;
   const read=file=>run(service.tool('rage-mod-cli'),[command,'read',file],{signal}).then(JSON.parse);
   const [original,current]=await Promise.all([read(raw(service,asset)),read(raw(service,asset,view))]);
   const before=command==='--car-spec'?original.fields:[{key:'manualOnly',value:original.manualOnly}],after=command==='--car-spec'?current.fields:[{key:'manualOnly',value:current.manualOnly}];
   const defaults=new Map(before.map(f=>[f.key,f.value]));
   for(const f of after){
    if(f.value===defaults.get(f.key))continue;
    const {stored,index}=binding(f.key);if(!stored)throw Error('Unknown raw car parameter: '+f.key);
    if(index===null)car[stored]=f.key==='manualOnly'?Boolean(f.value):f.value;
    else{if(!Array.isArray(car[stored]))throw Error('Missing car array: '+stored);car[stored][index]=f.value;}
    changed.add(f.key);fields.set(stored,car[stored]);
   }
  }
  if(changed.size){origins.set(car.id,changed);changes.set(car.id,fields);}
 }
 return {cars,origins,changes};
};
