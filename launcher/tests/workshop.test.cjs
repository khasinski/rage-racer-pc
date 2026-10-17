const {test}=require('node:test'),assert=require('node:assert/strict');
test('car sections separate everyday actions from technical details and retain every part action',async()=>{
 const {carWorkspace}=await import('../renderer/car-workshop.mjs');
 const car={key:'car.player.10.part.0',bank:10,part:0,rival:false,name:'A <custom> car'};
 const parts=Array.from({length:5},(_,part)=>({...car,part,key:`car.player.10.part.${part}`}));
 const appearance=carWorkspace(car,parts,{});
 assert.ok(appearance.includes('A &lt;custom&gt; car'));
 assert.ok(!appearance.includes('PLAYER SLOT'));
 assert.match(appearance,/data-export-set="car.player.10.part.0"/);
 assert.match(appearance,/data-import-set="car.player.10.part.0"/);
 assert.match(appearance,/data-import-set="car.player.10.part.0" data-format="glb"/);
 assert.match(appearance,/<details[^>]*><summary>OBJ sets and individual parts/);
 assert.match(appearance,/Import OBJ car set folder/);
 for(const part of parts)for(const action of ['preview','export','import'])assert.ok(appearance.includes(`data-${action}-car="${part.key}"`));
 assert.ok(!appearance.includes('data-car-spec='));
 const driving=carWorkspace(car,parts,{tab:'driving'});
 assert.match(driving,/data-car-spec="10"/);assert.ok(!driving.includes('data-export-set='));
 const files=carWorkspace(car,parts,{tab:'files',changes:'<p>Current mod</p>'});
 assert.match(files,/Current mod/);assert.match(files,/data-action="publish-mods"/);
 assert.match(files,/<details[^>]*><summary>Technical details/);
 assert.ok(!files.includes('data-car-spec='));
 const rival=carWorkspace({...car,rival:true},parts,{tab:'driving'});
 assert.ok(!rival.includes('data-car-spec='));assert.match(rival,/not a separate player specification/);
 assert.match(appearance,/id="car-inline-preview"/);
 assert.match(appearance,/data-inline-part="whole"/);
 for(const part of parts)assert.ok(appearance.includes(`data-inline-part="${part.key}"`));
});
test('lost car mappings show named required assignments and escape imported names',async()=>{
 const {carImportForm}=await import('../renderer/car-workshop.mjs');
 const html=carImportForm({token:'test',nodes:[{node:2,name:'<part>'}],materials:[{material:3,name:'<paint>'}],roles:[{instance:0,label:'Body'}],choices:[{word:0,label:'Body texture'}]});
 assert.match(html,/data-import-node="2"/);assert.match(html,/data-import-material="3"/);
 assert.match(html,/&lt;part&gt;/);assert.match(html,/&lt;paint&gt;/);
 assert.equal((html.match(/select required/g)||[]).length,2);assert.match(html,/Choose…/);assert.match(html,/Import mapped car/);
 assert.match(html,/data-preview-import="2"/);assert.match(html,/data-import-preview/);
 assert.match(html,/data-target-material="3"/);assert.match(html,/data-target-swatch="3" hidden/);
});
test('car workspace renders inline metadata inspector and torque curve chart when spec is loaded',async()=>{
 const {carWorkspace}=await import('../renderer/car-workshop.mjs');
 const car={key:'car.player.10.part.0',bank:10,part:0,rival:false,name:'Gnade Esperanza'};
 const parts=Array.from({length:5},(_,part)=>({...car,part,key:`car.player.10.part.${part}`}));
 const spec={revision:5,fields:[
  {key:'name',type:'text',value:'Gnade Esperanza',maxLength:31},
  {key:'manualOnly',value:0,baseValue:0},
  {key:'topGear',value:5,min:1,max:6},
  {key:'revLimit',value:8500,min:1000,max:15000},
  {key:'redline',value:7500,min:1000,max:15000},
  {key:'torqueBand0',value:1000,min:0,max:15000},
  {key:'torqueCurve0',value:220,min:0,max:5000}
 ]};
 const html=carWorkspace(car,parts,{spec});
 assert.match(html,/id="car-spec-form"[^>]*class="car-inline-spec"/);
 assert.match(html,/data-car-meta-tab="identity"/);
 assert.match(html,/data-car-meta-tab="drivetrain"/);
 assert.match(html,/data-car-meta-tab="handling"/);
 assert.match(html,/data-car-meta-tab="curves"/);
 assert.match(html,/class="torque-chart"/);
});
