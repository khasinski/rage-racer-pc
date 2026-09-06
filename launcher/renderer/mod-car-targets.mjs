export function modCarBanks(mod){
 const banks=new Set();
 for(const key of [...Object.keys(mod.manifest.materials||{}),...Object.keys(mod.manifest.textures||{})]){
  let match=/^car\.(\d+)\.material\./.exec(key);
  if(match&&Number(match[1])<32)banks.add('player:'+String(10+Number(match[1])*2));
  match=/^track\.(big|mid|hi|oval)([1-6])\.model-bank-1\.material\./.exec(key);
  if(match)banks.add('rival:'+String(88+((Number(match[2])-1)*4+['big','mid','hi','oval'].indexOf(match[1]))*2));
 }
 return banks;
}
export function modCarTargets(mod,parts){
 const banks=modCarBanks(mod);
 return parts.filter(p=>banks.has((p.rival?'rival':'player')+':'+p.bank)&&(p.rival?p.part%5===0:p.part===0));
}

export function carModChanges(mod,car){
 const family=car.rival?'rival':'player';
 const mesh=Object.keys(mod.manifest.meshes||{}).some(key=>{
  const match=/^car\.(player|rival)\.(\d+)\.part\.(\d+)$/.exec(key);
  if(!match||match[1]!==family||Number(match[2])!==car.bank)return false;
  const part=Number(match[3]);
  return car.rival?part>=car.part&&part<car.part+5:true;
 });
 const appearance=modCarBanks(mod).has(family+':'+car.bank);
 const raw=(mod.files||[]).some(file=>file==='raw/asset_'+String(car.bank).padStart(3,'0')+'.bin'||(!car.rival&&file==='raw/asset_'+String(car.bank+1).padStart(3,'0')+'.bin'));
 return {affected:mesh||appearance||raw,preview:mesh||appearance,raw};
}
