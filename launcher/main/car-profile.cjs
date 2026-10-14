const names={name:'name',manufacturer:'manufacturer',className:'class',steeringGripResponse:'steering_grip_response',steerResponse:'steer_response',referenceTurnRadius:'reference_turn_radius',negconSteeringAssistScale:'negcon_steering_assist_scale',speedDragDivisor:'speed_drag_divisor',baseSteeringGrip:'base_steering_grip',manualOnly:'manual_only',revLimit:'rev_limit',redline:'redline',topGear:'top_gear',automaticAccelerationScale:'automatic_acceleration_scale',price:'price',upgradePrice:'upgrade_price',unlockClass:'unlock_class'};
const arrays={torqueCurve:'torque_curve',torqueBand:'torque_band',torqueLossValue:'torque_loss_value',torqueLossRpm:'torque_loss_rpm',gearLoad:'gear_load',gearRatio:'gear_ratio',torqueScale:'torque_scale'};
function binding(key){
 const shift=/^(downshift|upshift)([1-6])$/.exec(key);if(shift)return {stored:'shift_points',index:(Number(shift[2])-1)*2+(shift[1]==='upshift'?1:0)};
 const array=/^(torqueCurve|torqueBand|torqueLossValue|torqueLossRpm|gearLoad|gearRatio|torqueScale)(\d+)$/.exec(key);return array?{stored:arrays[array[1]],index:Number(array[2])}:{stored:names[key],index:null};
}
async function read(service,bank){
  service.requireReady();service.carSpecFile(bank);const project=await service.project();
  const {run}=require('./service.cjs');
  return service.operation('Reading resolved car parameters',async signal=>require('./car-view.cjs').withView(service,signal,undefined,async view=>{
    const file=require('./car-view.cjs').rawSource(service,bank+1,view);
    const spec=JSON.parse(await run(service.tool('rage-mod-cli'),['--car-spec','read',file],{signal}));
    const model=require('./car-view.cjs').rawSource(service,bank,view);
    const transmission=JSON.parse(await run(service.tool('rage-mod-cli'),['--car-transmission','read',model],{signal}));
    const {cars,origins}=await require('./car-base.cjs')(service,view,signal);
    const car=cars.find(c=>c.variant===(bank-10)/2);if(!car)throw Error('Car is absent from this regional profile');
    spec.fields.unshift({key:'manualOnly',value:transmission.manualOnly,min:0,max:1});
    for(const [key,max]of [['price',2147483647],['upgradePrice',2147483647],['unlockClass',5]])if(Object.hasOwn(car,names[key]))spec.fields.push({key,value:car[names[key]],min:0,max});
    for(const key of ['name','manufacturer','className'])spec.fields.unshift({key,type:'text',value:car[names[key]],maxLength:31});
    const overrides=Object.fromEntries(project.changes.filter(c=>c.kind==='car'&&c.id===car.id).map(c=>[c.field,c.value]));
    for(const field of spec.fields){
      const {stored,index}=binding(field.key);
      const extract=value=>index!==null?value[index]:field.key==='manualOnly'?Number(value):value;
      field.origin='disc';
      if(Object.hasOwn(car,stored)){field.value=extract(car[stored]);field.origin=origins.get(car.id)?.has(field.key)?'selected packages':'built-in profile';}
      field.baseValue=field.value;
      if(Object.hasOwn(overrides,stored)){const v=extract(overrides[stored]);if(v!==field.baseValue){field.value=v;field.origin='project';}}
    }
    return {...spec,id:car.id,revision:project.revision};
  }));
}
async function edit(service,bank,edits,revision){
  if(!edits||typeof edits!=='object'||Array.isArray(edits)||!Object.keys(edits).length)throw Error('Invalid car parameters');
  const spec=await read(service,bank);
  service.editor.project.checkRevision(revision);
  const fields=new Map(spec.fields.map(f=>[f.key,f])),changes=[];
  const modified=new Map();
  for(const [key,value]of Object.entries(edits)){
    const f=fields.get(key);
    if(!f)throw Error('Invalid car parameter: '+key);
    if(f.type==='text'){
      if(typeof value!=='string'||!value.trim()||Buffer.byteLength(value,'utf8')>f.maxLength||/[#"\\\x00-\x1f\x7f]/.test(value))throw Error('Invalid car text: '+key+' (1–31 UTF-8 bytes, no #, quotes, backslashes or control characters)');
    }else if(!Number.isInteger(value)||value<f.min||value>f.max)throw Error('Invalid car parameter: '+key);
    const {stored,index}=binding(key);
    if(index!==null){
      if(!modified.has(stored)){
        const values=[],base=[];for(const field of spec.fields){const b=binding(field.key);if(b.stored===stored&&b.index!==null){values[b.index]=field.value;base[b.index]=field.baseValue;}}
        modified.set(stored,{values,base});
      }
      modified.get(stored).values[index]=value;
    }else changes.push({kind:'car',id:spec.id,field:stored,value:value===f.baseValue?null:key==='manualOnly'?Boolean(value):value});
  }
  for(const [field,{values,base}]of modified)changes.push({kind:'car',id:spec.id,field,value:values.every((v,i)=>v===base[i])?null:values});
  return service.editProject(changes,revision);
}
module.exports={read,edit,binding};
