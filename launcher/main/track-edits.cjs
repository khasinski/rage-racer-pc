const fs=require('node:fs/promises'),path=require('node:path');
function group(changes){
 const groups=new Map();
 for(const c of changes.filter(c=>c.kind==='track'&&c.field!=='mesh')){
  const match=/^track\.(big|mid|hi|oval)([1-6])\.(object|road|arc)\.(\d+)$/.exec(c.id);
  if(!match||!(match?.[3]==='arc'?['x','z']:match?.[3]==='road'?['x','y','z','angle','pitch','cross_slope','left_width','right_width','length','arc']:['x','y','z','yaw','model','flags']).includes(c.field)||!Number.isSafeInteger(c.value))throw Error('This track edit is not connected; no changes were applied');
  const bank=88+(Number(match[2])-1)*8+['big','mid','hi','oval'].indexOf(match[1])*2;
  if(!groups.has(bank))groups.set(bank,[]);
  groups.get(bank).push([Number(match[4]),(match[3]==='object'?'':match[3]+'.')+({left_width:'leftWidth',right_width:'rightWidth',cross_slope:'crossSlope'}[c.field]||c.field),c.value]);
 }
 return groups;
}
async function apply(service,changes,directory,files,signal){
 const {run}=require('./service.cjs');
 for(const [bank,edits]of group(changes)){
  const relative=`raw/asset_${String(bank).padStart(3,'0')}.bin`,target=path.join(directory,relative);
  const source=files.includes(relative)?target:path.join(service.state.disc.data,relative);
  await fs.mkdir(path.dirname(target),{recursive:true});
  const output=target+'.tmp';
  try{
   await run(service.tool('rage-mod-cli'),['--edit-track-stdin',source,output],{signal,input:Buffer.from(JSON.stringify(edits))});
   await fs.rename(output,target);if(!files.includes(relative))files.push(relative);
  }finally{await fs.rm(output,{force:true});}
 }
}
module.exports={group,apply};
