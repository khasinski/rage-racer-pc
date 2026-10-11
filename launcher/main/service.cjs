const fs = require('node:fs/promises');
const path = require('node:path');
const { spawn } = require('node:child_process');
const { randomUUID,createHash } = require('node:crypto');
const {Project}=require('./project.cjs');
const { defaults, validate, patchIni } = require('./config.cjs');

async function atomic(file, text) {
  await fs.mkdir(path.dirname(file),{recursive:true});
  const temp=file+'.'+randomUUID()+'.tmp';
  try { await fs.writeFile(temp,text);await fs.rename(temp,file); }
  finally { await fs.rm(temp,{force:true}); }
}
async function source(disc) {
  const hash=createHash('sha256'),handle=await fs.open(disc.archive,'r');
  let before,after;
  try{
    before=await handle.stat();
    for await(const bytes of handle.createReadStream({autoClose:false}))hash.update(bytes);
    after=await handle.stat();
  }finally{await handle.close();}
  if(before.size!==after.size||before.mtimeMs!==after.mtimeMs)throw Error('Source archive changed while reading it');
  const fingerprint='sha256:'+hash.digest('hex');
  if(disc.fingerprint&&disc.fingerprint!==fingerprint)throw Error('Source archive changed since extraction; import the disc again');
  return {region:disc.region,fingerprint};
}
function run(executable, args, {cwd,signal,onLog,input,maxOutput=2*1024*1024}={}) {
  return new Promise((resolve,reject)=>{
    const child=spawn(executable,args,{cwd,signal,windowsHide:true,shell:false});
    let forceTimer;
    const forceAfterGrace=()=>{
      if(forceTimer)return;
      forceTimer=setTimeout(()=>{
        if(child.exitCode===null&&child.signalCode===null)child.kill('SIGKILL');
      },5000);
      forceTimer.unref();
    };
    // A rejected native request may close stdin before consuming it. Its exit
    // status/stderr below remains authoritative; do not surface an uncaught EPIPE.
    child.stdin.on('error',()=>{});
    child.stdin.end(input);
    const chunks=[];let bytes=0,overflow=false,err='';
    child.stdout.on('data',d=>{
      if(!overflow){
        bytes+=d.length;
        if(bytes>maxOutput){overflow=true;chunks.length=0;child.kill();forceAfterGrace();}
        else chunks.push(d);
      }
      onLog?.(d.toString());
    });
    child.stderr.on('data',d=>{err=(err+d).slice(-32000);onLog?.(d.toString());});
    // Abort emits an error before the process has actually stopped. Wait for
    // close before deleting staging files that the child may still be writing.
    child.once('error',e=>{if(e.name==='AbortError')forceAfterGrace();else reject(e);});
    child.once('close',code=>{
      clearTimeout(forceTimer);
      if(signal?.aborted)reject(Error('Operation canceled'));
      else if(overflow)reject(Error(`Native tool output exceeds the ${maxOutput}-byte limit`));
      else if(code===0)resolve(Buffer.concat(chunks).toString('utf8'));
      else reject(Error(err||`Tool exited with code ${code}`));
    });
  });
}
class LauncherService {
  constructor({root,bin,config,modsRoot,onChange=()=>{}}) {
    this.root=root;this.bin=bin;this.baseConfig=config;this.modsRoot=modsRoot;this.onChange=onChange;
    this.state={settings:{...defaults},disc:null,mods:[]};this.busy=null;this.game=null;
  }
  tool(name) { return path.join(this.bin,name+(process.platform==='win32'?'.exe':'')); }
  async carTool(args,signal) {
    if(!(await this.snapshot()).ready)throw Error('The source image is unavailable. Select it again.');
    return run(this.tool('rage-racer'),['--config',this.baseConfig,'--set','disc.image='+this.state.disc.path,'--set','disc.choose=false',...args],{cwd:this.root,signal});
  }
  async cars() {
    this.requireReady();const project=await this.project();return this.operation('Reading car models',async signal=>{
      const catalog=JSON.parse(await this.carTool(['--set','tools.car_catalog=true'],signal));
      const names=JSON.parse(await run(this.tool('rage-save-cli'),['car-names'],{signal}));
      const display=require('./car-names.cjs');
      const profile=JSON.parse(await run(this.tool('rage-mod-cli'),['--car-profile',path.join(this.bin,this.state.disc.region==='NTSC-J'?'cars.ntscj.toml':'cars.toml')],{signal}));
      const authoredNames=new Map(project.changes.filter(c=>c.kind==='car'&&c.field==='name').map(c=>[c.id,c.value]));
      return {...catalog,parts:catalog.parts.map(part=>{const car=!part.rival&&part.part===0?profile.cars.find(c=>c.variant===(part.bank-10)/2):null;return {...part,name:car&&authoredNames.has(car.id)?authoredNames.get(car.id):display(part.name,this.state.disc.region,names)};})};
    });
  }
  async publishMods() {return require('./publish-mods.cjs')(this);}
  async gameModRoot() {
    const directory=this.modsRoot??JSON.parse(await run(this.tool('rage-mod-cli'),['--mod-directory'])).directory;
    if(typeof directory!=='string'||!path.isAbsolute(directory))throw Error('Invalid game mod directory');
    return directory;
  }
  async project() {
    this.requireReady();
    return this.operation('Opening editing project',async()=>{
      const project=await this.loadProject();
      if(!project)throw Error('Import the source archive before editing a project');
      return project.snapshot();
    },false);
  }
  // Internal operations call this while already owning the service job lock.
  async loadProject() {
    const disc=this.state.disc;if(!disc?.archive)return null;
    if(this.editor?.disc===disc)return this.editor.project;
    const identity=await source(disc),digest=identity.fingerprint.slice(7);
    const file=path.join(this.root,'projects',digest,'project.rrproject');
    const selected=this.modSelection();let project;
    try{project=await Project.open(file,identity,selected);}catch(e){if(e.code!=='ENOENT')throw e;project=new Project(identity,'My project',selected);}
    this.editor={disc,file,project};this.syncProjectSelection();return project;
  }
  modSelection() {
    return {packages:this.state.mods.filter(mod=>mod.enabled).map(mod=>mod.id),resolutions:this.state.resolutions||{}};
  }
  modsForSelection(selected) {
    return selected.packages.map(id=>{
      const mod=this.state.mods.find(mod=>mod.id===id);
      if(!mod)throw Error('Project package is unavailable: '+id+'. Remove this reference and select an imported replacement.');
      if(mod.region!==this.state.disc.region)throw Error('Project package belongs to a different disc region: '+mod.name);
      return {...mod,enabled:true};
    });
  }
  syncProjectSelection() {
    const selected=this.editor.project.snapshot().selection;
    for(const mod of this.state.mods)mod.enabled=selected.packages.includes(mod.id);
    this.state.resolutions=selected.resolutions;
  }
  async changeModSelection(input,revision) {
    const project=this.editor?.disc===this.state.disc?this.editor.project:null;
    if(project)project.checkRevision(revision);
    const selected=Project.selection(input);
    if(project)project.checkSize({...project.document,selection:selected});
    await this.validateModSelection(this.modsForSelection(selected));
    const enabled=this.state.mods.map(mod=>mod.enabled),resolutions=this.state.resolutions;
    for(const mod of this.state.mods)mod.enabled=selected.packages.includes(mod.id);
    this.state.resolutions=selected.resolutions;
    try{await this.persist();}catch(e){this.state.mods.forEach((mod,i)=>mod.enabled=enabled[i]);this.state.resolutions=resolutions;throw e;}
    if(project)return project.select(selected,revision);
  }
  async editProjectSelection(selected,revision) {
    this.requireReady();
    return this.operation('Updating project packages',async()=>{
      if(!await this.loadProject())throw Error('Open a source project first');
      return this.changeModSelection(selected,revision);
    },false);
  }
  async editProject(changes,revision) {
    if(this.busy)throw Error('Wait for the current operation to finish');
    if(!this.editor||this.editor.disc!==this.state.disc)throw Error('Open the project first');
    return this.editor.project.edit(changes,revision);
  }
  async projectHistory(direction,revision) {
    if(this.busy)throw Error('Wait for the current operation to finish');
    if(!this.editor||this.editor.disc!==this.state.disc)throw Error('Open the project first');
    return this.operation('Updating project history',()=>{
      const result=this.editor.project.history(direction,revision);this.syncProjectSelection();return result;
    },false);
  }
  async saveProject(revision) {
    if(!this.editor||this.editor.disc!==this.state.disc)throw Error('Open the project first');
    this.editor.project.checkRevision(revision);
    return this.operation('Saving editing project',()=>this.editor.project.save(this.editor.file),false);
  }
  async applyProject(revision) {
    if(this.game)throw Error('Close the game before applying project changes');
    if(!this.editor||this.editor.disc!==this.state.disc)throw Error('Open the project first');
    this.editor.project.checkRevision(revision);
    return this.operation('Building and applying project',async()=>{const result=await require('./project-build.cjs').apply(this);this.projectError=null;return result;},false);
  }
  async tracks() {
    this.requireReady();
    return this.operation('Reading original tracks',async signal=>{
      const names=['Mythical Coast','Over Pass City','Lakeside Gate','The Extreme Oval'];
      const tracks=[];
      for(let grade=1;grade<=6;grade++)for(let course=0;course<4;course++) {
        const bank=88+(grade-1)*8+course*2;
        const file=path.join(this.state.disc.data,'raw',`asset_${String(bank).padStart(3,'0')}.bin`);
        const pack=JSON.parse(await run(this.tool('rage-mod-cli'),['--track-pack',file],{signal}));
        tracks.push({id:`track/${['mythical-coast','over-pass-city','lakeside-gate','extreme-oval'][course]}/class/${grade}`,
          name:names[course],grade,bank,origin:'disc',available:course!==3||grade>=3,
          directions:['forward','reverse'],blocks:pack.blocks});
      }
      return {tracks};
    });
  }
  async previewTrackPart(id,part){return require('./track-meshes.cjs').previewPart(this,id,part);}
  async importTrackPart(id,part,file,revision){return require('./track-meshes.cjs').importPart(this,id,part,file,revision);}
  async exportTrackPart(id,part,directory){return require('./track-meshes.cjs').exportPart(this,id,part,directory);}
  async importTrackMesh(id,file,revision){return require('./track-meshes.cjs').importMesh(this,id,file,revision);}
  async exportTrackMesh(id,file){return require('./track-meshes.cjs').exportMesh(this,id,file);}
  async trackScene(bank,preview=false) {
    this.requireReady();
    if(!Number.isInteger(bank)||bank<88||bank>134||bank%2)throw Error('Invalid track');
    return this.operation('Reading track scene',async signal=>{
      await this.loadProject();
      const snapshot=this.editor?.project.snapshot(),selection=snapshot?.selection;
      let directory='';
      try{
        if(selection?.packages.length)directory=await this.composeMods(this.modsForSelection(selection),selection.resolutions);
        const files=directory?await require('./mods.cjs').inventory(directory,this.tool('rage-mod-cli')):[];
        if(!directory){directory=path.join(this.root,'track-preview-'+randomUUID());await fs.mkdir(directory);}
        const prefix=`track.${['big','mid','hi','oval'][((bank-88)/2)%4]}${Math.floor((bank-88)/8)+1}.`;
        const changes=(snapshot?.changes||[]).filter(c=>c.kind==='track'&&c.id.startsWith(prefix));
        await require('./track-edits.cjs').apply(this,changes,directory,files,signal);
        const relative=`raw/asset_${String(bank).padStart(3,'0')}.bin`;
        const file=path.join(files.includes(relative)?directory:this.state.disc.data,relative);
        if(preview){
          const metadata=JSON.parse(await run(this.tool('rage-racer'),['--config',this.baseConfig,'--set','tools.track_export='+file,'--set','tools.track_bank='+bank,'--set','tools.track_output='+directory],{cwd:this.root,signal,maxOutput:4*1024*1024}));
          const track=prefix.slice(0,-1);
          for(const name of ['course','terrain'])await require('./track-meshes.cjs').model(this,track+'.'+name,path.join(directory,name+'.rmesh'),directory,files,signal,metadata[name].meshes);
          const banks={};
          for(const name of ['course','terrain'])banks[name]={geometry:JSON.parse(await run(this.tool('rage-mod-cli'),['--preview-bank',path.join(directory,name+'.rmesh')],{signal,maxOutput:64*1024*1024})),instances:metadata[name].instances};
          await require('./track-textures.cjs')(this,bank,banks,metadata,directory,files,signal);
          return require('./track-preview.cjs')(banks);
        }
        return {...JSON.parse(await run(this.tool('rage-mod-cli'),['--track-scene',file],{signal,maxOutput:8*1024*1024})),revision:snapshot?.revision};
      }finally{if(directory)await fs.rm(directory,{recursive:true,force:true});}
    });
  }
  async saveTrackObject(bank,index,values,revision){return this.saveTrackEntry(bank,index,values,revision,'object');}
  async saveRoadPoint(bank,index,values,revision){return this.saveTrackEntry(bank,index,values,revision,'road');}
  async saveRoadArc(bank,index,values,revision){return this.saveTrackEntry(bank,index,values,revision,'arc');}
  async saveTerrainCell(bank,index,values,revision){return this.saveTrackEntry(bank,index,values,revision,'cell');}
  async saveTrackEntry(bank,index,values,revision,type){
    this.requireReady();
    if(!Number.isInteger(bank)||bank<88||bank>134||bank%2||!Number.isInteger(index)||index<0||index>10000||!values||typeof values!=='object'||Array.isArray(values)||!Object.keys(values).length||Object.keys(values).length>(type==='road'?10:6))throw Error('Invalid track object edit');
    return this.operation(type==='object'?'Editing track object':'Editing road',async signal=>{
      await this.loadProject();if(!this.editor)throw Error('Open a source project first');
      const project=this.editor.project;project.checkRevision(revision);
      const id=`track.${['big','mid','hi','oval'][((bank-88)/2)%4]}${Math.floor((bank-88)/8)+1}.${type}.${index}`;
      const changes=Object.entries(values).map(([field,value])=>({kind:'track',id,field,value}));
      require('./track-edits.cjs').group(changes);
      let directory='';
      try{
        const snapshot=project.snapshot(),selection=snapshot.selection;
        const prefix=id.slice(0,id.indexOf('.'+type+'.'))+'.';
        const effective=new Map([...snapshot.changes.filter(c=>c.kind==='track'&&c.id.startsWith(prefix)),...changes].map(c=>[c.id+':'+c.field,c]));
        if(selection.packages.length)directory=await this.composeMods(this.modsForSelection(selection),selection.resolutions);
        const files=directory?await require('./mods.cjs').inventory(directory,this.tool('rage-mod-cli')):[];
        if(!directory){directory=path.join(this.root,'track-edit-'+randomUUID());await fs.mkdir(directory);}
        await require('./track-edits.cjs').apply(this,[...effective.values()],directory,files,signal);
        if(signal.aborted)throw Error('Operation canceled');
        return project.edit(changes,revision);
      }finally{if(directory)await fs.rm(directory,{recursive:true,force:true});}
    });
  }
  carSpecFile(bank) {
    if(!Number.isInteger(bank)||bank<10||bank>72||bank%2)throw Error('Invalid player car bank');
    return path.join(this.state.disc.data,'raw',`asset_${String(bank+1).padStart(3,'0')}.bin`);
  }
  async carSpec(bank) {return require('./car-profile.cjs').read(this,bank);}
  async saveCarSpec(bank,edits,revision) {return require('./car-profile.cjs').edit(this,bank,edits,revision);}
  async exportCar(key,destination) {
    this.requireReady();if(typeof key!=='string'||!/^car\.(player|rival)\.\d+\.part\.\d+$/.test(key)||key.length>95)throw Error('Invalid car part');
    return this.operation('Exporting car model',signal=>this.exportCarData(key,destination,signal));
  }
  async exportCarData(key,destination,signal,format='png',view){
    return require('./car-view.cjs').withView(this,signal,view,async mod=>{
      const target=path.join(destination,key);await fs.mkdir(target);
      try{
        const mesh=path.join(target,'model.rmesh');
        const metadata=JSON.parse(await this.carTool(['--set','tools.car_export='+key,'--set','tools.car_output='+mesh],signal));
        await require('./car-view.cjs').model(this,key,mesh,mod);
        await run(this.tool('rage-mesh-obj'),['export',mesh,'0',path.join(target,'model.obj')],{signal});
        const geometry=JSON.parse(await run(this.tool('rage-mod-cli'),['--preview',mesh],{signal,maxOutput:16*1024*1024}));
        const appearance=await require('./preview-materials.cjs')(this,key,metadata,mod,signal);
        const baseMetadata={...metadata,materials:metadata.materials.filter(m=>!appearance[m.source]?.texture)};
        const textures=await require('./preview-textures.cjs')(this,key,geometry,baseMetadata,target,signal,format);
        for(const [source,material]of Object.entries(appearance))if(material.texture){
          const name='texture-'+source+'.png';
          await fs.writeFile(path.join(target,name),Buffer.from(material.texture.png,'base64'));textures[source]=name;
        }
        const materials=[...new Set(geometry.vertices.filter((_,i)=>i%9===6))];
        const mtl=materials.map(word=>{
          const id=word&0xffff,surface=id>>>6,texture=textures[id%64];
          const color=id===65535?'0.15 0.16 0.18':surface===1?'0.1 0.16 0.2':'1 1 1';
          return `newmtl rage_${word}\nKd ${color}\nd 1\n${texture&&surface!==1&&id!==65535?'map_Kd '+texture+'\n':''}`;
        }).join('\n');
        await fs.writeFile(path.join(target,'model.obj.mtl'),mtl);
        if(format==='png')await fs.writeFile(path.join(target,'materials.json'),JSON.stringify(metadata.materials));
        await fs.writeFile(path.join(target,'README.txt'),'Current model and textures: original disc, selected packages, then project edits.\nKeep the OBJ, MTL and PNG textures together. Lighting and glass are simplified.\nEdit model.obj in a 3D editor. Preserve rage_* material names and export triangulated OBJ with UVs and normals. Use Import car set to import shared texture edits; Import model only changes geometry.\n');
        return target;
      }catch(e){await fs.rm(target,{recursive:true,force:true});throw e;}
    });
  }
  async exportCarSet(key,destination){return require('./car-set.cjs').exportSet(this,key,destination);}
  async importCarSet(key,source){return require('./car-set.cjs').importSet(this,key,source);}
  async importCar(key,file) {
    this.requireReady();if(typeof key!=='string'||!/^car\.(player|rival)\.\d+\.part\.\d+$/.test(key)||key.length>95)throw Error('Invalid car part');
    return this.operation('Importing car model',async signal=>{
      const catalog=JSON.parse(await this.carTool(['--set','tools.car_catalog=true'],signal));
      const part=catalog.parts.find(p=>p.key===key);if(!part)throw Error('Unknown car part');
      const staging=path.join(this.root,'car-import-'+randomUUID());
      await fs.mkdir(path.join(staging,'meshes'),{recursive:true});
      try{
        const mesh=path.join(staging,'meshes/model.rmesh');
        const extension=path.extname(file).toLowerCase();
        if(extension==='.obj')await run(this.tool('rage-mesh-obj'),['import',file,mesh],{signal});
        else if(extension==='.rmesh')await fs.copyFile(file,mesh);
        else throw Error('Choose an OBJ or RRMESH file');
        await require('./car-material-bindings.cjs')(this,key,mesh,staging,path.basename(file),signal);
        await this.loadProject();
        if(this.editor){
          const project=this.editor.project,revision=project.revision;
          const reference=await new (require('./project-files.cjs').ProjectFiles)(this.editor.file).put(mesh);
          return project.edit([{kind:'car',id:key,field:'mesh',value:reference}],revision);
        }
        await fs.writeFile(path.join(staging,'mod.toml'),`[mod]\nid = "car-replacement"\n[meshes]\n"${key}" = "meshes/model.rmesh"\n`);
        const names=JSON.parse(await run(this.tool('rage-save-cli'),['car-names'],{signal}));
        const display=require('./car-names.cjs')(part.name,this.state.disc.region,names);
        const mod=await this.installModFiles(staging,signal,`${display} · part ${part.part} · ${path.basename(file)}`);
        return mod.id;
      }finally{await fs.rm(staging,{recursive:true,force:true});}
    });
  }
  async previewCar(key,modId) {
    this.requireReady();if(typeof key!=='string'||!/^car\.(player|rival)\.\d+\.part\.\d+$/.test(key)||key.length>95)throw Error('Invalid car part');
    return this.operation('Loading model preview',signal=>this.previewCarData(key,modId,signal));
  }
  async carMaterials(key){
    this.requireReady();
    if(typeof key!=='string'||!/^car\.(player|rival)\.\d+\.part\.\d+$/.test(key)||key.length>95)throw Error('Invalid car part');
    return this.operation('Reading car materials',signal=>require('./car-view.cjs').withView(this,signal,undefined,async view=>{
      const staging=path.join(this.root,'car-materials-'+randomUUID());await fs.mkdir(staging);
      try{
        const metadata=JSON.parse(await this.carTool(['--set','tools.car_export='+key,'--set','tools.car_output='+path.join(staging,'model.rmesh')],signal));
        const [,kind,bank]=key.split('.');
        const table=JSON.parse(await run(this.tool('rage-mod-cli'),['--car-materials',kind,path.join(this.state.disc.data,'raw',`asset_${bank.padStart(3,'0')}.bin`),bank],{signal}));
        const appearance=await require('./preview-materials.cjs')(this,key,metadata,view,signal);
        return {revision:this.editor?.project.revision,materials:metadata.materials.flatMap(m=>{
          const slot=table.materials.find(s=>s.page===m.page&&s.clut===m.clut);
          return slot?[{id:slot.id,name:'Texture '+(slot.slot+1),properties:appearance[m.source]?.properties||'lit opaque 0.35 0 1 1 1 1 0 0 0'}]:[];
        })};
      }finally{await fs.rm(staging,{recursive:true,force:true});}
    }));
  }
  async previewCarData(key,modId,signal,appearanceModId=modId,view) {
    const selected=view??(appearanceModId===undefined?undefined:this.state.mods.find(m=>m.id===appearanceModId));
    if(appearanceModId!==undefined&&!selected)throw Error('Unknown mod model');
    return require('./car-view.cjs').withView(this,signal,selected,async mod=>{
      const staging=path.join(this.root,'car-preview-'+randomUUID());await fs.mkdir(staging);
      try{
        let file=path.join(staging,'model.rmesh');
        const metadata=JSON.parse(await this.carTool(['--set','tools.car_export='+key,'--set','tools.car_output='+file],signal));
        if(modId!==undefined&&!mod.manifest.meshes?.[key])throw Error('Unknown mod model');
        await require('./car-view.cjs').model(this,key,file,mod);
        const geometry=JSON.parse(await run(this.tool('rage-mod-cli'),['--preview',file],{signal,maxOutput:16*1024*1024}));
        geometry.materials=await require('./preview-materials.cjs')(this,key,metadata,mod,signal);
        geometry.textures=await require('./preview-textures.cjs')(this,key,geometry,{...metadata,materials:metadata.materials.filter(m=>!geometry.materials[m.source]?.texture)},staging,signal);
        for(const [source,material]of Object.entries(geometry.materials))if(material.texture)geometry.textures[source]=material.texture;
        return geometry;
      }finally{await fs.rm(staging,{recursive:true,force:true});}
    });
  }
  async previewWholeCar(bank,body=0,rival=false,modId) {
    this.requireReady();
    if(typeof rival!=='boolean'||!Number.isInteger(body)||body<0||body>30||body%5)throw Error('Invalid car variant');
    if(rival){if(!Number.isInteger(bank)||bank<88||bank>134||bank%2)throw Error('Invalid rival bank');}
    else{this.carSpecFile(bank);if(body!==0)throw Error('Invalid player body');}
    const mod=modId===undefined?null:this.state.mods.find(m=>m.id===modId);
    if(modId!==undefined&&!mod)throw Error('Unknown mod');
    return this.operation('Loading car preview',signal=>require('./car-view.cjs').withView(this,signal,modId===undefined?undefined:mod,async view=>{
      const file=path.join(this.state.disc.data,'raw',`asset_${String(bank).padStart(3,'0')}.bin`);
      const layout=JSON.parse(await run(this.tool('rage-mod-cli'),rival?['--rival-layout',file,String(body)]:['--car-layout',file],{signal}));
      const parts=new Map();
      for(const {part}of layout.instances)if(!parts.has(part)){
        const key=`car.${rival?'rival':'player'}.${bank}.part.${part}`;
        parts.set(part,await this.previewCarData(key,mod?.manifest.meshes?.[key]?modId:undefined,signal,modId,view));
      }
      const geometry=require('./assemble-preview.cjs')(layout,parts);
      if(mod)geometry.modName=mod.name;
      return geometry;
    }));
  }
  async init() {
    await fs.mkdir(this.root,{recursive:true});
    try { const state=JSON.parse(await fs.readFile(path.join(this.root,'launcher.json'),'utf8'));this.state={...this.state,...state,settings:{...defaults,...validate(state.settings)}}; }
    catch(e) { if(e.code!=='ENOENT')this.startupError=`Could not load launcher settings: ${e.message}`; }
    try{await this.refreshLegacyTextures();}catch(e){this.startupError=`Could not read installed texture mods: ${e.message}`;for(const mod of this.state.mods)mod.enabled=false;}
    try{await this.loadProject();}catch(e){this.projectError=`Could not open editing project: ${e.message}`;}
    return this.snapshot();
  }
  async persist() { await atomic(path.join(this.root,'launcher.json'),JSON.stringify(this.state,null,2)); }
  async snapshot() {
    let ready=false;
    if(this.state.disc)try{await fs.access(this.state.disc.path);await fs.access(path.join(this.state.disc.data,'manifest.json'));ready=true;}catch{}
    let toolsReady=true;for(const name of ['rage-racer','rage-extract','rage-pack','rage-save-cli','rage-mod-cli','rage-mesh-obj'])try{await fs.access(this.tool(name));}catch{toolsReady=false;}
    return {...this.state,project:this.editor?.disc===this.state.disc?this.editor.project.snapshot():null,ready,toolsReady,overlaps:this.overlaps(),conflicts:this.conflicts(),busy:this.busy?.label||null,canCancel:!!this.busy?.cancellable,running:!!this.game,error:this.startupError||this.projectError||this.gameError||null};
  }
  async update() {this.onChange(await this.snapshot());}
  async logs() {
    const entries=[];
    for(const name of ['game-process.log','game.log']){
      let file;
      try{
        file=await fs.open(path.join(this.root,name),'r');
        const size=(await file.stat()).size,start=Math.max(0,size-65536),buffer=Buffer.alloc(size-start);
        const {bytesRead}=await file.read(buffer,0,buffer.length,start);
        entries.push({name,text:buffer.subarray(0,bytesRead).toString('utf8'),truncated:start>0});
      }catch(e){if(e.code!=='ENOENT')throw e;entries.push({name,text:'',missing:true});}
      finally{if(file)await file.close();}
    }
    return entries;
  }
  requireReady() {if(!this.state.disc)throw Error('Add a game image first');if(this.busy)throw Error('Wait for the current operation');}
  async operation(label,task,cancellable=true) {
    if(this.busy||this.game)throw Error('Another operation or the game is running');
    const controller=new AbortController();this.busy={label,controller,cancellable};
    try{await this.update();return await task(controller.signal);}finally{this.busy=null;await this.update();}
  }
  cancel() {if(this.busy?.cancellable)this.busy.controller.abort();}
  async prepare(image) {
    if(!['.cue','.bin','.chd'].includes(path.extname(image).toLowerCase()))throw Error('Select CUE, CHD or Track 01 BIN');
    return this.operation('Reading disc image',async signal=>{
      const staging=path.join(this.root,'games',randomUUID());await fs.mkdir(staging,{recursive:true});
      try {
        const config=path.join(staging,'import.ini');await fs.writeFile(config,'[video]\nrenderer = modern\n');
        const archive=path.join(staging,'RAGE.BIN');
        const stdout=await run(this.tool('rage-racer'),['--config',config,'--set',`disc.image=${image}`,'--set',`tools.dump_archive=${archive}`,'--set','tools.launcher_report=true','--set',`diagnostics.log=${path.join(staging,'import.log')}`],{cwd:staging,signal});
        const line=stdout.split(/\r?\n/).find(x=>x.startsWith('{"region":'));
        const report=line?JSON.parse(line):null,region=report?.region;
        if(!['PAL','NTSC-U','NTSC-J'].includes(region))throw Error('The selected image is not a recognized Rage Racer release');
        this.busy.label='Preparing the asset library';await this.update();
        const data=path.join(staging,'data');await run(this.tool('rage-extract'),[archive,data],{cwd:staging,signal});
        await fs.access(path.join(data,'menu_music.wav'));
        const manifest=JSON.parse(await fs.readFile(path.join(data,'manifest.json'),'utf8'));
        if(!Array.isArray(manifest.entries)||manifest.entries.length!==135)throw Error('Archive extraction did not produce a complete library');
        for(const entry of manifest.entries)if(entry.present!==false){
          if(entry.raw!==`raw/asset_${String(entry.index).padStart(3,'0')}.bin`)throw Error('Invalid extracted entry path');
          if((await fs.stat(path.join(data,entry.raw))).size!==entry.size)throw Error('An extracted asset is incomplete');
        }
        if(signal.aborted)throw Error('Operation canceled');
        this.busy.cancellable=false;await this.update();
        const previous=this.state.disc;
        const identity=await source({archive,region});
        if(report.fingerprint&&report.fingerprint!==identity.fingerprint)throw Error('Native disc fingerprint does not match the extracted archive');
        this.state.disc={path:image,region,data,archive,fingerprint:identity.fingerprint};
        // A different regional archive cannot inherit active overrides implicitly.
        const previousMods=this.state.mods;this.state.mods=this.state.mods.map(m=>({...m,enabled:false}));
        try{await this.persist();}catch(e){this.state.disc=previous;this.state.mods=previousMods;throw e;}
        return this.snapshot();
      } catch(e) {await fs.rm(staging,{recursive:true,force:true});throw e;}
    });
  }
  async saveSettings(values) {
    this.requireReady();
    await this.operation('Saving settings',async()=>{
      const previous=this.state.settings;
      this.state.settings={...previous,...validate(values)};
      try{await this.persist();}catch(e){this.state.settings=previous;throw e;}
    },false);
    return this.snapshot();
  }
  async configuration(modDirectory='', ownedJob=false) {
    if(!ownedJob)this.requireReady();let text='';
    try{text=await fs.readFile(path.join(this.root,'rage-port.ini'),'utf8');}catch(e){if(e.code!=='ENOENT')throw e;text=await fs.readFile(this.baseConfig,'utf8');}
    const style=this.state.disc.region==='NTSC-J'?'japanese':'international';
    this.projectError=null;
    try{
      const build=await require('./project-build.cjs').activeBuild(this);
      // Pass the pointer root, not a direct profile: runtime validates and
      // selects both providers together against the mounted disc.
      if(build)modDirectory=await this.gameModRoot();
    }catch(e){this.projectError='Project activation failed: '+e.message;modDirectory='';}
    return patchIni(text,{...this.state.settings,'video.renderer':'modern','disc.image':this.state.disc.path,'disc.choose':false,'mods.directory':modDirectory,'cars.profile':'','audio.menu_music':path.join(this.state.disc.data,'menu_music.wav'),'content.car_names':style,'content.prologue':style,'diagnostics.log':path.join(this.root,'game.log')});
  }
  async launch() {
    this.requireReady();
    return this.operation('Starting game', async signal => {
      if(!(await this.snapshot()).ready)throw Error('The source image is unavailable. Select it again.');
      const menuMusic=path.join(this.state.disc.data,'menu_music.wav');
      try{await fs.access(menuMusic);}catch{
        this.busy.label='Repairing the asset library';await this.update();
        await run(this.tool('rage-extract'),[this.state.disc.archive,this.state.disc.data],{cwd:this.root,signal});
        await fs.access(menuMusic);
        this.busy.label='Starting game';await this.update();
      }
      // Imported source libraries launch the committed project (or originals),
      // never unsaved package choices. Legacy profiles lack a source archive.
      const active=this.state.disc.archive?'':await this.composeMods();
      const config=path.join(this.root,'rage-port.ini');
      let log;
      try {
        // configuration() requires an idle service; the launch operation already
        // owns the job lock, so use the explicit internal writer here.
        await atomic(config,await this.configuration(active,true));
        log=await fs.open(path.join(this.root,'game-process.log'),'a');
        const child=spawn(this.tool('rage-racer'),['--config',config,'--set','video.renderer=modern'],{cwd:this.root,windowsHide:true,shell:false});
        child.stdout.on('data',d=>log.write(d).catch(()=>{}));
        child.stderr.on('data',d=>log.write(d).catch(()=>{}));
        await new Promise((resolve,reject)=>{child.once('spawn',resolve);child.once('error',reject);});
        this.game=child;
        this.gameError=null;
        child.once('close',(code,signal)=>{
          this.game=null;
          if(code||signal)this.gameError=`The game ${signal?'was stopped by '+signal:'exited with code '+code}. See game-process.log and game.log.`;
          // Cleanup failures must not retain a dead game or reject an async
          // EventEmitter callback without a consumer.
          void Promise.allSettled([log.close(),active?fs.rm(active,{recursive:true,force:true}):Promise.resolve()])
            .then(()=>this.update()).catch(()=>{});
        });
      } catch(e) {
        // Each owned resource gets a cleanup attempt even if another fails.
        // Preserve the launch error rather than replacing it with a log error.
        await Promise.allSettled([log?log.close():Promise.resolve(),
          active?fs.rm(active,{recursive:true,force:true}):Promise.resolve()]);
        throw e;
      }
    },false);
  }
  async newSave(destination,region) {
    if(this.game)throw Error('Close the game before editing its saves');
    if(!['PAL','NTSC-U','NTSC-J'].includes(region))throw Error('Invalid save region');
    return this.operation('Creating a save',async()=>{
      const temp=destination+'.'+randomUUID()+'.tmp';
      try {
        const result=JSON.parse(await run(this.tool('rage-save-cli'),['new',temp,region]));
        await fs.rename(temp,destination);
        return {...result,region};
      }finally{await fs.rm(temp,{force:true});}
    },false);
  }
  async saves() {return JSON.parse(await run(this.tool('rage-save-cli'),['discover']));}
  async readSave(file,cardIndex) {const args=['read',file];if(cardIndex!==undefined)args.push('--card',String(cardIndex));return JSON.parse(await run(this.tool('rage-save-cli'),args));}
  async writeSave(file,destination,cardIndex,edits) {
    if(this.busy)throw Error('Wait for the current operation to finish');
    if(this.game)throw Error('Close the game before editing its saves');
    if(path.resolve(file)===path.resolve(destination))throw Error('Choose a separate output file to preserve the original');
    const args=['write',file],temp=destination+'.'+randomUUID()+'.tmp';args.push(temp);
    if(cardIndex!==undefined)args.push('--card',String(cardIndex));
    if(!edits||typeof edits!=='object'||Object.keys(edits).length>1500)throw Error('Invalid edits');
    for(const [key,value]of Object.entries(edits)){if(typeof value!=='string'&&typeof value!=='number')throw Error('Invalid field value');args.push(key,String(value));}
    return this.operation('Saving a save copy',async()=>{
      try {
        const sourceInfo=await fs.stat(file);
        let destinationInfo;
        try{destinationInfo=await fs.stat(destination);}catch(e){if(e.code!=='ENOENT')throw e;}
        if(destinationInfo&&sourceInfo.dev===destinationInfo.dev&&sourceInfo.ino===destinationInfo.ino)
          throw Error('Choose a separate output file to preserve the original');
        const result=JSON.parse(await run(this.tool('rage-save-cli'),args));
        await fs.rename(temp,destination);
        return result;
      }finally{await fs.rm(temp,{force:true});}
    },false);
  }
  async assets() {this.requireReady();return JSON.parse(await fs.readFile(path.join(this.state.disc.data,'manifest.json'),'utf8'));}
  async exportAsset(index,destination) {this.requireReady();if(!Number.isInteger(index)||index<0||index>=135)throw Error('Invalid asset');await fs.copyFile(path.join(this.state.disc.data,'raw',`asset_${String(index).padStart(3,'0')}.bin`),destination);}
}
require('./mods.cjs').installMethods(LauncherService);
module.exports={LauncherService,run,atomic,source};
