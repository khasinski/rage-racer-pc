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
// Shape validation at the JSON boundary; graph policy lives in the compiled
// ModSelectionBuildOrder implementation shared with runtime TOML selection.
module.exports={readDependencies};
