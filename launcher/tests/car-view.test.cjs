const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {withView,model}=require('../main/car-view.cjs');
test('one car view owns its package snapshot and cleans it after success or failure',async t=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-car-view-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));
 let compositions=0;
 const service={root,loadProject:async()=>{},editor:{project:{snapshot:()=>({selection:{packages:['chosen'],resolutions:{}},changes:[]})}},modsForSelection:selection=>selection.packages,
  tool:name=>path.resolve(__dirname,'../resources/bin',name+(process.platform==='win32'?'.exe':'')),
  composeMods:async packages=>{
   assert.deepEqual(packages,['chosen']);compositions++;
   const folder=path.join(root,'snapshot');await fs.mkdir(path.join(folder,'meshes'),{recursive:true});
   await fs.writeFile(path.join(folder,'meshes/model.rmesh'),'immutable snapshot');
   await fs.writeFile(path.join(folder,'mod.toml'),'[meshes]\n"car.player.10.part.0" = "meshes/model.rmesh"\n');return folder;
  }};
 await withView(service,new AbortController().signal,undefined,async view=>{
  for(let i=0;i<3;i++){
   const output=path.join(root,'part-'+i);await model(service,'car.player.10.part.0',output,view);
   assert.equal(await fs.readFile(output,'utf8'),'immutable snapshot');
  }
 });
 assert.equal(compositions,1);await assert.rejects(fs.stat(path.join(root,'snapshot')),{code:'ENOENT'});
 await assert.rejects(withView(service,new AbortController().signal,undefined,async()=>{throw Error('preview failed');}),/preview failed/);
 await assert.rejects(fs.stat(path.join(root,'snapshot')),{code:'ENOENT'});
 const explicit={id:'comparison',manifest:{},files:[]};
 await withView(service,null,explicit,view=>assert.equal(view,explicit));assert.equal(compositions,2);
});
test('project model wins over the package while explicit comparison preserves package bytes',async t=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-car-precedence-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));
 const key='car.player.10.part.0',file=path.join(root,'project.rrproject'),input=path.join(root,'input');
 await fs.mkdir(path.join(root,'meshes'));await fs.writeFile(path.join(root,'meshes/package.rmesh'),'package');await fs.writeFile(input,'project');
 const reference=await new (require('../main/project-files.cjs').ProjectFiles)(file).put(input);
 const service={root,editor:{file,project:{snapshot:()=>({changes:[{kind:'car',id:key,field:'mesh',value:reference}]})}}};
 const view={root,project:true,files:['meshes/package.rmesh'],manifest:{meshes:{[key]:'meshes/package.rmesh'}}},target=path.join(root,'resolved');
 await model(service,key,target,view);assert.equal(await fs.readFile(target,'utf8'),'project');
 await model(service,key,target,{...view,project:false});assert.equal(await fs.readFile(target,'utf8'),'package');
});
