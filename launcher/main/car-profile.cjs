const path=require('node:path');
const names={steeringGripResponse:'steering_grip_response',steerResponse:'steer_response',referenceTurnRadius:'reference_turn_radius',negconSteeringAssistScale:'negcon_steering_assist_scale',speedDragDivisor:'speed_drag_divisor',baseSteeringGrip:'base_steering_grip',manualOnly:'manual_only',revLimit:'rev_limit',redline:'redline',topGear:'top_gear',automaticAccelerationScale:'automatic_acceleration_scale',price:'price',upgradePrice:'upgrade_price',unlockClass:'unlock_class'};
async function read(service,bank){
  service.requireReady();const file=service.carSpecFile(bank),project=await service.project();
  const {run}=require('./service.cjs');
  return service.operation('Reading resolved car parameters',async signal=>{
    const spec=JSON.parse(await run(service.tool('rage-mod-cli'),['--car-spec','read',file],{signal}));
    const model=path.join(path.dirname(file),`asset_${String(bank).padStart(3,'0')}.bin`);
    const transmission=JSON.parse(await run(service.tool('rage-mod-cli'),['--car-transmission','read',model],{signal}));
    const base=path.join(service.bin,service.state.disc.region==='NTSC-J'?'cars.ntscj.toml':'cars.toml');
    const {cars}=JSON.parse(await run(service.tool('rage-mod-cli'),['--car-profile',base],{signal}));
    const car=cars.find(c=>c.variant===(bank-10)/2);if(!car)throw Error('Car is absent from this regional profile');
    spec.fields.unshift({key:'manualOnly',value:transmission.manualOnly,min:0,max:1});
    for(const [key,max]of [['price',2147483647],['upgradePrice',2147483647],['unlockClass',5]])if(Object.hasOwn(car,names[key]))spec.fields.push({key,value:car[names[key]],min:0,max});
    const overrides=Object.fromEntries(project.changes.filter(c=>c.kind==='car'&&c.id===car.id).map(c=>[c.field,c.value]));
    for(const field of spec.fields){
      const shift=/^(downshift|upshift)([1-6])$/.exec(field.key);
      const stored=shift?'shift_points':names[field.key];
      const index=shift?(Number(shift[2])-1)*2+(shift[1]==='upshift'?1:0):null;
      const extract=value=>shift?value[index]:field.key==='manualOnly'?Number(value):value;
      field.origin='disc';
      if(Object.hasOwn(car,stored)){field.value=extract(car[stored]);field.origin='built-in profile';}
      field.baseValue=field.value;
      if(Object.hasOwn(overrides,stored)){field.value=extract(overrides[stored]);field.origin='project';}
    }
    return {...spec,id:car.id,revision:project.revision};
  });
}
async function edit(service,bank,edits,revision){
  if(!edits||typeof edits!=='object'||Array.isArray(edits)||!Object.keys(edits).length)throw Error('Invalid car parameters');
  const spec=await read(service,bank);
  service.editor.project.checkRevision(revision);
  const fields=new Map(spec.fields.map(f=>[f.key,f])),changes=[];
  let shifted=false;
  const points=Array.from({length:12},(_,i)=>fields.get((i%2?'upshift':'downshift')+(Math.floor(i/2)+1)).value);
  for(const [key,value]of Object.entries(edits)){
    const f=fields.get(key);
    if(!f||!Number.isInteger(value)||value<f.min||value>f.max)throw Error('Invalid car parameter: '+key);
    const shift=/^(downshift|upshift)([1-6])$/.exec(key);
    if(shift){points[(Number(shift[2])-1)*2+(shift[1]==='upshift'?1:0)]=value;shifted=true;}
    else changes.push({kind:'car',id:spec.id,field:names[key],value:value===f.baseValue?null:key==='manualOnly'?Boolean(value):value});
  }
  if(shifted)changes.push({kind:'car',id:spec.id,field:'shift_points',value:points.every((v,i)=>v===fields.get((i%2?'upshift':'downshift')+(Math.floor(i/2)+1)).baseValue)?null:points});
  return service.editProject(changes,revision);
}
module.exports={read,edit};
