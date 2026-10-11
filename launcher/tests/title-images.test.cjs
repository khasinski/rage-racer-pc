const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {LauncherService,run}=require('../main/service.cjs');
test('title menu editor names all six buttons, distinguishes states and provides PNG actions',async()=>{
 const {interfaceWorkspace}=await import('../renderer/interface.mjs');
 const images=['Grand Prix','Extra Grand Prix','Time Attack','Custom','Load / Save','Options'].flatMap(label=>[false,true].map(selected=>({id:'ui.title.custom'+(selected?'.selected':''),label,selected,origin:'built-in',png:'test',overridden:selected})));
 const html=interfaceWorkspace({revision:9,images});for(const label of ['Grand Prix','Time Attack','Custom','Load / Save','Options'])assert.ok(html.includes(label));
 assert.equal([...html.matchAll(/data-import-title-image=/g)].length,12);assert.equal([...html.matchAll(/data-export-title-image=/g)].length,12);assert.match(html,/112×16/);assert.match(html,/Alpha below 128/);assert.match(html,/data-revision="9"/);assert.match(html,/data-reset-title-image="ui.title.custom"[^>]*disabled/);
});
test('title image export/import/reset uses shared project history, selected state and immutable PNGs',async t=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-title-editor-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));
 const archive=path.join(root,'RAGE.BIN'),config=path.join(root,'default.ini');await fs.writeFile(archive,'title source');await fs.writeFile(config,'[video]\nrenderer=modern\n');
 const s=new LauncherService({root,bin:path.resolve(__dirname,'../resources/bin'),config,modsRoot:path.join(root,'mods')});await s.init();s.requireReady=()=>{};s.state.disc={archive,path:archive,region:'PAL',data:root};
 const original=await s.titleImages();assert.equal(original.images.length,12);assert.equal(new Set(original.images.map(i=>i.id)).size,12);
 const normal=original.images.find(i=>i.id==='ui.title.custom'),selected=original.images.find(i=>i.id==='ui.title.custom.selected');assert.notEqual(normal.png,selected.png);
 const file=path.join(root,'custom.png');await s.exportTitleImage(selected.id,file);assert.equal((await fs.readFile(file)).toString('base64'),selected.png);
 let p=await s.importTitleImage(normal.id,file,original.revision);await fs.writeFile(file,'damage source after import');
 let read=await s.titleImages();assert.equal(read.images.find(i=>i.id===normal.id).png,selected.png);assert.equal(read.images.find(i=>i.id===normal.id).origin,'project');
 await assert.rejects(s.importTitleImage(normal.id,file,p.revision),/Choose|decode|failed|exit/i);assert.equal((await s.project()).revision,p.revision);
 await assert.rejects(s.resetTitleImage('../bad',p.revision),/Unknown/);await assert.rejects(s.resetTitleImage(normal.id,original.revision),/changed/);
 p=await s.resetTitleImage(normal.id,p.revision);assert.equal((await s.titleImages()).images.find(i=>i.id===normal.id).png,normal.png);
 p=await s.projectHistory('undo',p.revision);assert.equal((await s.titleImages()).images.find(i=>i.id===normal.id).png,selected.png);
 const output=path.join(root,'exclusive.png');await run(s.tool('rage-mod-cli'),['--title-image','3','0',output]);const prior=await fs.readFile(output);await assert.rejects(run(s.tool('rage-mod-cli'),['--title-image','3','1',output]));assert.deepEqual(await fs.readFile(output),prior);
});
