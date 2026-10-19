const fs=require('node:fs/promises'),path=require('node:path');
function group(changes){
 const groups=new Map();
 for(const c of changes.filter(c=>c.kind==='track'&&!['mesh','texture','material'].includes(c.field))){
  const match=/^track\.(?:(big|mid|hi|oval)([1-6])|custom([1-9]|[12][0-9]|3[0-2]))\.(object|road|arc|cell|env|palette|environment|ai|speed|start|camera|intro)\.(\d+)$/.exec(c.id);
  const type=match?.[4];
  if(!match||!(type==='intro'?['x','y','z','frame','duration']:type==='camera'?['mode','section','duration','data_0','data_1','data_2','data_3','offset_0','offset_1','offset_2']:type==='start'?['x','z','point']:type==='speed'?['progress','pitch','target_0','target_1','target_2','target_3']:type==='ai'?['start','end','min_offset','max_offset','adjustment']:type==='environment'?['length','sky_row']:type==='palette'?Array.from({length:16},(_,i)=>'color_'+i):type==='env'?['time','duration','mode',...Array.from({length:9},(_,i)=>'color_'+i)]:type==='cell'?['model']:type==='arc'?['x','z']:type==='road'?['x','y','z','angle','pitch','cross_slope','left_width','right_width','length','arc']:['x','y','z','yaw','model','flags']).includes(c.field)||!Number.isSafeInteger(c.value))throw Error('This track edit is not connected; no changes were applied');
  const bank=match[3]?136+(Number(match[3])-1)*2:88+(Number(match[2])-1)*8+['big','mid','hi','oval'].indexOf(match[1])*2;
  if(!groups.has(bank))groups.set(bank,[]);
  groups.get(bank).push([Number(match[5]),(type==='object'?'':type+'.')+({left_width:'leftWidth',right_width:'rightWidth',cross_slope:'crossSlope'}[c.field]||c.field),c.value]);
 }
 for(const edits of groups.values())edits.sort((a,b)=>a[0]-b[0]);
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
