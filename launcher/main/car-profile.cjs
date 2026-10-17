const names={name:'name',manufacturer:'manufacturer',className:'class',steeringGripResponse:'steering_grip_response',steerResponse:'steer_response',referenceTurnRadius:'reference_turn_radius',negconSteeringAssistScale:'negcon_steering_assist_scale',speedDragDivisor:'speed_drag_divisor',baseSteeringGrip:'base_steering_grip',manualOnly:'manual_only',revLimit:'rev_limit',redline:'redline',topGear:'top_gear',automaticAccelerationScale:'automatic_acceleration_scale',price:'price',upgradePrice:'upgrade_price',unlockClass:'unlock_class'};
const arrays={torqueCurve:'torque_curve',torqueBand:'torque_band',torqueLossValue:'torque_loss_value',torqueLossRpm:'torque_loss_rpm',gearLoad:'gear_load',gearRatio:'gear_ratio',torqueScale:'torque_scale'};
function binding(key){
 const shift=/^(downshift|upshift)([1-6])$/.exec(key);if(shift)return {stored:'shift_points',index:(Number(shift[2])-1)*2+(shift[1]==='upshift'?1:0)};
 const array=/^(torqueCurve|torqueBand|torqueLossValue|torqueLossRpm|gearLoad|gearRatio|torqueScale)(\d+)$/.exec(key);return array?{stored:arrays[array[1]],index:Number(array[2])}:{stored:names[key],index:null};
}
async function read(service,bank){
  service.requireReady();
  const isCustomSlot=Number.isInteger(bank)&&bank>=200&&bank<=998&&bank%2===0;
  if(!isCustomSlot)service.carSpecFile(bank);
  const project=await service.project();
  const donorBank=isCustomSlot?(Number(project.changes.find(c=>c.kind==='car'&&c.id===`custom_car_${bank}`&&c.field==='donor_bank')?.value)||10):bank;
  if(isCustomSlot)service.carSpecFile(donorBank);
  const {run}=require('./service.cjs');
  return service.operation('Reading resolved car parameters',async signal=>require('./car-view.cjs').withView(service,signal,undefined,async view=>{
    const file=require('./car-view.cjs').rawSource(service,donorBank+1,view);
    const spec=JSON.parse(await run(service.tool('rage-mod-cli'),['--car-spec','read',file],{signal}));
    const model=require('./car-view.cjs').rawSource(service,donorBank,view);
    const transmission=JSON.parse(await run(service.tool('rage-mod-cli'),['--car-transmission','read',model],{signal}));
    const {cars,origins}=await require('./car-base.cjs')(service,view,signal);
    const car=cars.find(c=>c.variant===(donorBank-10)/2);if(!car)throw Error('Car is absent from this regional profile');
    const recordId=isCustomSlot?`custom_car_${bank}`:car.id;
    spec.fields.unshift({key:'manualOnly',value:transmission.manualOnly,min:0,max:1});
    for(const [key,max]of [['price',2147483647],['upgradePrice',2147483647],['unlockClass',5]])if(Object.hasOwn(car,names[key]))spec.fields.push({key,value:car[names[key]],min:0,max});
    for(const key of ['name','manufacturer','className'])spec.fields.unshift({key,type:'text',value:car[names[key]],maxLength:31});
    const overrides=Object.fromEntries(project.changes.filter(c=>c.kind==='car'&&c.id===recordId).map(c=>[c.field,c.value]));
    for(const field of spec.fields){
      const {stored,index}=binding(field.key);
      const extract=value=>index!==null?value[index]:field.key==='manualOnly'?Number(value):value;
      field.origin='disc';
      if(Object.hasOwn(car,stored)){field.value=extract(car[stored]);field.origin=origins.get(car.id)?.has(field.key)?'selected packages':'built-in profile';}
      field.baseValue=field.value;
      if(Object.hasOwn(overrides,stored)){const v=extract(overrides[stored]);if(v!==field.baseValue||isCustomSlot){field.value=v;field.origin='project';}}
    }
    return {...spec,id:recordId,donorBank,customSlot:isCustomSlot,revision:project.revision};
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
    }else changes.push({kind:'car',id:spec.id,field:stored,value:(value===f.baseValue&&!spec.customSlot)?null:key==='manualOnly'?Boolean(value):value});
  }
  for(const [field,{values,base}]of modified)changes.push({kind:'car',id:spec.id,field,value:(values.every((v,i)=>v===base[i])&&!spec.customSlot)?null:values});
  return service.editProject(changes,revision);
}
async function createNew(service,options,revision){
  if(!options||typeof options!=='object')throw Error('Invalid new car options');
  const sourceBank=Number(options.sourceBank??10);
  service.carSpecFile(sourceBank);
  const project=await service.project();
  let targetBank;
  if(options.targetBank==='new'||options.targetBank===undefined||options.targetBank===null||Number(options.targetBank)>=200){
    if(Number.isInteger(Number(options.targetBank))&&Number(options.targetBank)>=200&&Number(options.targetBank)%2===0){
      targetBank=Number(options.targetBank);
    }else{
      let maxBank=198;
      for(const c of project.changes||[]){
        const m=/^custom_car_(\d+)$/.exec(c.id);
        if(m){const b=Number(m[1]);if(b>maxBank&&b%2===0)maxBank=b;}
      }
      targetBank=maxBank+2;
    }
    const recordId=`custom_car_${targetBank}`;
    await service.editProject([{kind:'car',id:recordId,field:'donor_bank',value:sourceBank}],revision);
    revision=service.editor.project.revision;
  }else{
    targetBank=Number(options.targetBank);
    service.carSpecFile(targetBank);
  }
  const targetSpec=await read(service,targetBank);
  const sourceSpec=sourceBank===targetBank?targetSpec:await read(service,sourceBank);
  const edits={};
  if(sourceBank!==targetBank){
    const targetMap=new Map(targetSpec.fields.map(f=>[f.key,f]));
    for(const sf of sourceSpec.fields){
      const tf=targetMap.get(sf.key);
      if(!tf||sf.type==='text')continue;
      const clamped=Math.max(tf.min,Math.min(tf.max,Number(sf.value)));
      if(Number.isInteger(clamped))edits[sf.key]=clamped;
    }
  }
  for(const key of ['name','manufacturer','className']){
    if(typeof options[key]==='string'&&options[key].trim())edits[key]=options[key].trim();
  }
  for(const key of ['price','upgradePrice','unlockClass','manualOnly']){
    if(options[key]!==undefined&&options[key]!==null&&options[key]!==''){
      const tf=targetSpec.fields.find(f=>f.key===key);
      if(tf)edits[key]=Math.max(tf.min,Math.min(tf.max,Math.round(Number(options[key]))));
    }
  }
  if(!Object.keys(edits).length)edits.name=targetSpec.fields.find(f=>f.key==='name')?.value||'NEW CAR';
  const updated=await edit(service,targetBank,edits,revision);
  return {...updated,createdBank:targetBank};
}
module.exports={read,edit,createNew,binding};
