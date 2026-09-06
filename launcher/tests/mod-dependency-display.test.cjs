const {test}=require('node:test'),assert=require('node:assert/strict');
test('the mods list can say which installed mod satisfies a requirement',async()=>{
 const {duplicatePackageIds,requirementStatus,requirementNote,requirementLabel}=await import('../renderer/mod-dependencies.mjs');
 const dependency={packageId:'core',version:'1.0'};
 const addon={id:'a',name:'Addon',region:'PAL',enabled:false,requires:[dependency]};
 const core={id:'c',name:'Core',packageId:'core',version:'1.0',region:'PAL',enabled:false};
 const otherVersion={id:'v',name:'Core 2',packageId:'core',version:'2.0',region:'PAL',enabled:true};
 const otherRegion={id:'r',name:'Core US',packageId:'core',version:'1.0',region:'NTSC-U',enabled:true};

 assert.equal(requirementLabel(dependency),'core (1.0)');
 assert.equal(requirementLabel({packageId:'core'}),'core');

 // Nothing installed matches: wrong version and wrong region do not count.
 let status=requirementStatus(addon,dependency,[addon,otherVersion,otherRegion]);
 assert.equal(status.state,'missing');assert.equal(requirementNote(status),'not installed');

 // Installed but disabled names the mod to enable first.
 status=requirementStatus(addon,dependency,[addon,core,otherVersion]);
 assert.equal(status.state,'available');assert.deepEqual(status.mods,[core]);
 assert.equal(requirementNote(status),'enable Core first');

 // Exactly one enabled match satisfies it.
 core.enabled=true;
 status=requirementStatus(addon,dependency,[addon,core]);
 assert.equal(status.state,'satisfied');assert.equal(requirementNote(status),'satisfied by Core');

 // A requirement without a version accepts any version, and two enabled copies
 // are reported the way activation rejects them.
 status=requirementStatus(addon,{packageId:'core'},[addon,core,otherVersion]);
 assert.equal(status.state,'multiple');assert.equal(requirementNote(status),'multiple enabled copies');

 // A mod never satisfies its own requirement, and a legacy mod without a
 // packageId is identified by its id.
 const legacy={id:'core',name:'Legacy core',region:'PAL',enabled:true};
 assert.equal(requirementStatus(legacy,{packageId:'core'},[legacy]).state,'missing');
 assert.equal(requirementStatus(addon,{packageId:'core'},[addon,legacy]).state,'satisfied');

 assert.deepEqual([...duplicatePackageIds([addon,core,otherVersion,otherRegion])],['core']);
 assert.deepEqual([...duplicatePackageIds([addon,core])],[]);
});
