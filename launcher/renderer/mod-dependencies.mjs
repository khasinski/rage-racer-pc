// Display-side view of mod dependencies. The main process decides what may be
// enabled (main/mod-dependencies.cjs); this mirrors its matching rules so the
// list can say which installed mod satisfies a requirement before the player
// tries to enable anything.
export function packageIdOf(mod){return mod.packageId||mod.id;}

// Package IDs used by more than one installed mod, so a requirement could be
// satisfied by either copy and enabling both is refused.
export function duplicatePackageIds(mods){
 const counts=new Map();
 for(const mod of mods){const id=packageIdOf(mod);counts.set(id,(counts.get(id)||0)+1);}
 return new Set([...counts].filter(([,count])=>count>1).map(([id])=>id));
}

// state: 'satisfied' (exactly one enabled match), 'multiple' (several enabled
// matches, which activation rejects), 'available' (installed but disabled) or
// 'missing'. `mods` lists every matching installed mod in list order.
export function requirementStatus(mod,dependency,installed){
 const matches=installed.filter(other=>other!==mod&&packageIdOf(other)===dependency.packageId&&other.region===mod.region&&(dependency.version===undefined||dependency.version===other.version));
 const enabled=matches.filter(other=>other.enabled);
 if(enabled.length===1)return {state:'satisfied',mods:enabled};
 if(enabled.length>1)return {state:'multiple',mods:enabled};
 if(matches.length)return {state:'available',mods:matches};
 return {state:'missing',mods:[]};
}

export function requirementLabel(dependency){return dependency.packageId+(dependency.version?' ('+dependency.version+')':'');}

// Short English note appended to a requirement in the list.
export function requirementNote(status){
 switch(status.state){
 case 'satisfied':return 'satisfied by '+status.mods[0].name;
 case 'multiple':return 'multiple enabled copies';
 case 'available':return 'enable '+status.mods[0].name+' first';
 default:return 'not installed';
 }
}
