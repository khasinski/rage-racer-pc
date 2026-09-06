const fs = require('node:fs/promises');
const path = require('node:path');
const { spawn } = require('node:child_process');
const { randomUUID } = require('node:crypto');
const { defaults, validate, patchIni } = require('./config.cjs');

async function atomic(file, text) {
  await fs.mkdir(path.dirname(file),{recursive:true});
  const temp=file+'.'+randomUUID()+'.tmp';
  try { await fs.writeFile(temp,text);await fs.rename(temp,file); }
  finally { await fs.rm(temp,{force:true}); }
}
function run(executable, args, {cwd,signal,onLog,input,maxOutput=2*1024*1024}={}) {
  return new Promise((resolve,reject)=>{
    const child=spawn(executable,args,{cwd,signal,windowsHide:true,shell:false});
    // A rejected native request may close stdin before consuming it. Its exit
    // status/stderr below remains authoritative; do not surface an uncaught EPIPE.
    child.stdin.on('error',()=>{});
    child.stdin.end(input);
    const chunks=[];let bytes=0,overflow=false,err='';
    child.stdout.on('data',d=>{
      if(!overflow){
        bytes+=d.length;
        if(bytes>maxOutput){overflow=true;chunks.length=0;child.kill();}
        else chunks.push(d);
      }
      onLog?.(d.toString());
    });
    child.stderr.on('data',d=>{err=(err+d).slice(-32000);onLog?.(d.toString());});
    // Abort emits an error before the process has actually stopped. Wait for
    // close before deleting staging files that the child may still be writing.
    child.once('error',e=>{if(e.name!=='AbortError')reject(e);});
    child.once('close',code=>{
      if(signal?.aborted)reject(Error('Operation canceled'));
      else if(overflow)reject(Error(`Native tool output exceeds the ${maxOutput}-byte limit`));
      else if(code===0)resolve(Buffer.concat(chunks).toString('utf8'));
      else reject(Error(err||`Tool exited with code ${code}`));
    });
  });
}
class LauncherService {
  constructor({root,bin,config,onChange=()=>{}}) {
    this.root=root;this.bin=bin;this.baseConfig=config;this.onChange=onChange;
    this.state={settings:{...defaults},disc:null,mods:[]};this.busy=null;this.game=null;
  }
  tool(name) { return path.join(this.bin,name+(process.platform==='win32'?'.exe':'')); }
  async carTool(args,signal) {
    if(!(await this.snapshot()).ready)throw Error('The source image is unavailable. Select it again.');
    return run(this.tool('rage-racer'),['--config',this.baseConfig,'--set','disc.image='+this.state.disc.path,'--set','disc.choose=false',...args],{cwd:this.root,signal});
  }
  async cars() {
    this.requireReady();return this.operation('Reading car models',async signal=>{
      const catalog=JSON.parse(await this.carTool(['--set','tools.car_catalog=true'],signal));
      const names=JSON.parse(await run(this.tool('rage-save-cli'),['car-names'],{signal}));
      const display=require('./car-names.cjs');
      return {...catalog,parts:catalog.parts.map(part=>({...part,name:display(part.name,this.state.disc.region,names)}))};
    });
  }
  carSpecFile(bank) {
    if(!Number.isInteger(bank)||bank<10||bank>72||bank%2)throw Error('Invalid player car bank');
    return path.join(this.state.disc.data,'raw',`asset_${String(bank+1).padStart(3,'0')}.bin`);
  }
  async carSpec(bank) {
    this.requireReady();const file=this.carSpecFile(bank);
    return this.operation('Reading car parameters',async signal=>{
      const spec=JSON.parse(await run(this.tool('rage-mod-cli'),['--car-spec','read',file],{signal}));
      const model=path.join(path.dirname(file),`asset_${String(bank).padStart(3,'0')}.bin`);
      const transmission=JSON.parse(await run(this.tool('rage-mod-cli'),['--car-transmission','read',model],{signal}));
      spec.fields.unshift({key:'manualOnly',value:transmission.manualOnly,min:0,max:1});return spec;
    });
  }
  async saveCarSpec(bank,edits) {
    this.requireReady();const file=this.carSpecFile(bank);
    const allowed=['manualOnly','revLimit','redline','topGear','automaticAccelerationScale',...Array.from({length:6},(_,i)=>['downshift'+(i+1),'upshift'+(i+1)]).flat()];
    if(!edits||typeof edits!=='object'||Array.isArray(edits)||!Object.keys(edits).length||
       Object.entries(edits).some(([key,value])=>!allowed.includes(key)||!Number.isInteger(value)))throw Error('Invalid car parameters');
    return this.operation('Saving car parameters as a mod',async signal=>{
      const catalog=JSON.parse(await this.carTool(['--set','tools.car_catalog=true'],signal));
      const part=catalog.parts.find(p=>p.bank===bank&&!p.rival&&p.part===0);
      if(!part)throw Error('Unknown player car');
      const staging=path.join(this.root,'car-spec-'+randomUUID());
      await fs.mkdir(path.join(staging,'raw'),{recursive:true});
      try{
        const specEdits=Object.entries(edits).filter(([k])=>k!=='manualOnly');
        if(specEdits.length)await run(this.tool('rage-mod-cli'),['--car-spec','write',file,path.join(staging,'raw',path.basename(file)),...specEdits.flatMap(([k,v])=>[k,String(v)])],{signal});
        if(Object.hasOwn(edits,'manualOnly')){
          const name=`asset_${String(bank).padStart(3,'0')}.bin`;
          await run(this.tool('rage-mod-cli'),['--car-transmission','write',path.join(path.dirname(file),name),path.join(staging,'raw',name),String(edits.manualOnly)],{signal});
        }
        const names=JSON.parse(await run(this.tool('rage-save-cli'),['car-names'],{signal}));
        const name=require('./car-names.cjs')(part.name,this.state.disc.region,names);
        const mod=await this.installModFiles(staging,signal,`${name} · parameters`);
        return mod.id;
      }finally{await fs.rm(staging,{recursive:true,force:true});}
    });
  }
  async exportCar(key,destination) {
    this.requireReady();if(typeof key!=='string'||!/^car\.(player|rival)\.\d+\.part\.\d+$/.test(key)||key.length>95)throw Error('Invalid car part');
    return this.operation('Exporting car model',signal=>this.exportCarData(key,destination,signal));
  }
  async exportCarData(key,destination,signal,format='tga'){
      const target=path.join(destination,key);await fs.mkdir(target);
      try{
        const mesh=path.join(target,'model.rmesh');
        const metadata=JSON.parse(await this.carTool(['--set','tools.car_export='+key,'--set','tools.car_output='+mesh],signal));
        await run(this.tool('rage-mesh-obj'),['export',mesh,'0',path.join(target,'model.obj')],{signal});
        const geometry=JSON.parse(await run(this.tool('rage-mod-cli'),['--preview',mesh],{signal,maxOutput:16*1024*1024}));
        const textures=await require('./preview-textures.cjs')(this,key,geometry,metadata,target,signal,format);
        const materials=[...new Set(geometry.vertices.filter((_,i)=>i%9===6))];
        const mtl=materials.map(word=>{
          const id=word&0xffff,surface=id>>>6,texture=textures[id%64];
          const color=id===65535?'0.15 0.16 0.18':surface===1?'0.1 0.16 0.2':'1 1 1';
          return `newmtl rage_${word}\nKd ${color}\nd 1\n${texture&&surface!==1&&id!==65535?'map_Kd '+texture+'\n':''}`;
        }).join('\n');
        await fs.writeFile(path.join(target,'model.obj.mtl'),mtl);
        if(format==='png')await fs.writeFile(path.join(target,'materials.json'),JSON.stringify(metadata.materials));
        await fs.writeFile(path.join(target,'README.txt'),'Authored base model, before enabled mod overrides.\nKeep the OBJ, MTL and TGA textures together. Textures are decoded from your selected game image; lighting and glass are simplified.\nEdit model.obj in a 3D editor. Preserve rage_* material names and export triangulated OBJ with UVs and normals. Texture edits are not imported by the mesh replacement action.\n');
        return target;
      }catch(e){await fs.rm(target,{recursive:true,force:true});throw e;}
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
  async previewCarData(key,modId,signal,appearanceModId=modId) {
      const staging=path.join(this.root,'car-preview-'+randomUUID());await fs.mkdir(staging);
      try{
        let file=path.join(staging,'model.rmesh');
        const metadata=JSON.parse(await this.carTool(['--set','tools.car_export='+key,'--set','tools.car_output='+file],signal));
        if(modId!==undefined){
          const mod=this.state.mods.find(m=>m.id===modId),relative=mod?.manifest.meshes?.[key];
          if(!relative||!mod.files.includes(relative)||!/^meshes\/[A-Za-z0-9_./-]+\.rmesh$/.test(relative)||relative.split('/').includes('..'))throw Error('Unknown mod model');
          file=path.join(this.root,'mods',mod.id,relative);
        }
        const geometry=JSON.parse(await run(this.tool('rage-mod-cli'),['--preview',file],{signal,maxOutput:16*1024*1024}));
        geometry.textures=await require('./preview-textures.cjs')(this,key,geometry,metadata,staging,signal);
        geometry.materials=await require('./preview-materials.cjs')(this,key,metadata,this.state.mods.find(m=>m.id===appearanceModId),signal);
        for(const [source,material]of Object.entries(geometry.materials))if(material.texture)geometry.textures[source]=material.texture;
        return geometry;
      }finally{await fs.rm(staging,{recursive:true,force:true});}
  }
  async previewWholeCar(bank,body=0,rival=false,modId) {
    this.requireReady();
    if(typeof rival!=='boolean'||!Number.isInteger(body)||body<0||body>30||body%5)throw Error('Invalid car variant');
    if(rival){if(!Number.isInteger(bank)||bank<88||bank>134||bank%2)throw Error('Invalid rival bank');}
    else{this.carSpecFile(bank);if(body!==0)throw Error('Invalid player body');}
    const mod=modId===undefined?null:this.state.mods.find(m=>m.id===modId);
    if(modId!==undefined&&!mod)throw Error('Unknown mod');
    return this.operation('Loading car preview',async signal=>{
      const file=path.join(this.state.disc.data,'raw',`asset_${String(bank).padStart(3,'0')}.bin`);
      const layout=JSON.parse(await run(this.tool('rage-mod-cli'),rival?['--rival-layout',file,String(body)]:['--car-layout',file],{signal}));
      const parts=new Map();
      for(const {part}of layout.instances)if(!parts.has(part)){
        const key=`car.${rival?'rival':'player'}.${bank}.part.${part}`;
        parts.set(part,await this.previewCarData(key,mod?.manifest.meshes?.[key]?modId:undefined,signal,modId));
      }
      const geometry=require('./assemble-preview.cjs')(layout,parts);
      if(mod)geometry.modName=mod.name;
      return geometry;
    });
  }
  async init() {
    await fs.mkdir(this.root,{recursive:true});
    try { const state=JSON.parse(await fs.readFile(path.join(this.root,'launcher.json'),'utf8'));this.state={...this.state,...state,settings:{...defaults,...validate(state.settings)}}; }
    catch(e) { if(e.code!=='ENOENT')this.startupError=`Could not load launcher settings: ${e.message}`; }
    try{await this.refreshLegacyTextures();}catch(e){this.startupError=`Could not read installed texture mods: ${e.message}`;for(const mod of this.state.mods)mod.enabled=false;}
    return this.snapshot();
  }
  async persist() { await atomic(path.join(this.root,'launcher.json'),JSON.stringify(this.state,null,2)); }
  async snapshot() {
    let ready=false;
    if(this.state.disc)try{await fs.access(this.state.disc.path);await fs.access(path.join(this.state.disc.data,'manifest.json'));ready=true;}catch{}
    let toolsReady=true;for(const name of ['rage-racer','rage-extract','rage-pack','rage-save-cli','rage-mod-cli','rage-mesh-obj'])try{await fs.access(this.tool(name));}catch{toolsReady=false;}
    return {...this.state,ready,toolsReady,overlaps:this.overlaps(),conflicts:this.conflicts(),busy:this.busy?.label||null,canCancel:!!this.busy?.cancellable,running:!!this.game,error:this.startupError||this.gameError||null};
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
        const region=line?JSON.parse(line).region:null;
        if(!['PAL','NTSC-U','NTSC-J'].includes(region))throw Error('The selected image is not a recognized Rage Racer release');
        this.busy.label='Preparing the asset library';await this.update();
        const data=path.join(staging,'data');await run(this.tool('rage-extract'),[archive,data],{cwd:staging,signal});
        const manifest=JSON.parse(await fs.readFile(path.join(data,'manifest.json'),'utf8'));
        if(!Array.isArray(manifest.entries)||manifest.entries.length!==135)throw Error('Archive extraction did not produce a complete library');
        for(const entry of manifest.entries)if(entry.present!==false){
          if(entry.raw!==`raw/asset_${String(entry.index).padStart(3,'0')}.bin`)throw Error('Invalid extracted entry path');
          if((await fs.stat(path.join(data,entry.raw))).size!==entry.size)throw Error('An extracted asset is incomplete');
        }
        if(signal.aborted)throw Error('Operation canceled');
        this.busy.cancellable=false;await this.update();
        const previous=this.state.disc;
        this.state.disc={path:image,region,data,archive};
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
    return patchIni(text,{...this.state.settings,'video.renderer':'modern','disc.image':this.state.disc.path,'disc.choose':false,'mods.directory':modDirectory,'content.car_names':style,'content.prologue':style,'diagnostics.log':path.join(this.root,'game.log')});
  }
  async launch() {
    this.requireReady();
    return this.operation('Starting game', async () => {
      if(!(await this.snapshot()).ready)throw Error('The source image is unavailable. Select it again.');
      const active=await this.composeMods();
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
        if(log)await log.close();
        if(active)await fs.rm(active,{recursive:true,force:true});
        throw e;
      }
    },false);
  }
  async saves() {this.requireReady();return JSON.parse(await run(this.tool('rage-save-cli'),['discover']));}
  async readSave(file,cardIndex) {this.requireReady();const args=['read',file];if(cardIndex!==undefined)args.push('--card',String(cardIndex));return JSON.parse(await run(this.tool('rage-save-cli'),args));}
  async writeSave(file,destination,cardIndex,edits) {
    this.requireReady();if(this.game)throw Error('Close the game before editing its saves');
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
module.exports={LauncherService,run,atomic};
