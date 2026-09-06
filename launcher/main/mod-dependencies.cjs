const identifier=value=>typeof value==='string'&&/^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$/.test(value);
function readDependencies(metadata){
 const packageId=metadata.packageId,requires=metadata.requires??[];
 if(packageId!==undefined&&!identifier(packageId))throw Error('Invalid mod packageId');
 if(!Array.isArray(requires)||requires.length>32)throw Error('Invalid mod dependencies');
 const seen=new Set();
 for(const dependency of requires){
  if(!dependency||typeof dependency!=='object'||!identifier(dependency.packageId)||dependency.packageId===packageId||seen.has(dependency.packageId)||Object.keys(dependency).some(k=>!['packageId','version'].includes(k)))throw Error('Invalid mod dependency');
  if(dependency.version!==undefined&&(typeof dependency.version!=='string'||!dependency.version.trim()||dependency.version.length>80||/[\x00-\x1f\x7f]/.test(dependency.version)))throw Error('Invalid dependency version');
  seen.add(dependency.packageId);
 }
 return {packageId,requires};
}
function dependencyIssues(mods){
 const active=mods.filter(m=>m.enabled),issues=[];
 for(const mod of active)for(const dependency of mod.requires||[]){
  const matches=active.filter(other=>(other.packageId||other.id)===dependency.packageId&&other.region===mod.region&&(dependency.version===undefined||dependency.version===other.version));
  if(matches.length!==1)issues.push({id:mod.id,message:mod.name+' requires '+dependency.packageId+(dependency.version?' version '+dependency.version:'')+(matches.length?' (multiple enabled copies)':' (enable a matching mod first)')});
 }
 return issues;
}
function assertDependencies(mods){const issues=dependencyIssues(mods);if(issues.length)throw Error(issues.map(i=>i.message).join('\n'));}
module.exports={readDependencies,dependencyIssues,assertDependencies};
