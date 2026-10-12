const {app,BrowserWindow,dialog,ipcMain,Menu}=require('electron');
const path=require('node:path');
const {pathToFileURL}=require('node:url');
const {LauncherService}=require('./service.cjs');
let window,service,savePaths=new Map(),currentSave;
const hidden=app.commandLine.hasSwitch('hidden');
if(hidden&&process.platform==='darwin')app.setActivationPolicy('prohibited');
const page=path.join(__dirname,'../renderer/index.html');
const resources=app.isPackaged?path.join(process.resourcesPath,'resources'):path.join(__dirname,'../resources');
const testRoot=process.env.RAGE_LAUNCHER_USER_DATA;
const userData=app.commandLine.getSwitchValue('user-data-dir');
if(userData)app.setPath('userData',path.resolve(userData));
else if(testRoot&&!app.isPackaged)app.setPath('userData',testRoot);
else if(!app.commandLine.hasSwitch('user-data-dir')){
  // Keep existing profiles when changing the visible product name.
  app.setPath('userData',path.join(app.getPath('appData'),'Rage Racer Launcher'));
}
if(!app.requestSingleInstanceLock())app.quit();
else {
app.on('second-instance',()=>{if(!hidden){window?.show();window?.focus();}});
function handle(name,fn){ipcMain.handle('launcher:'+name,async(event,...args)=>{
  if(event.sender!==window?.webContents||event.senderFrame!==window.webContents.mainFrame||event.senderFrame.url!==pathToFileURL(page).href)throw Error('Untrusted request');
  try{return{ok:true,value:await fn(...args)};}catch(e){return{ok:false,error:e.message};}
});}
async function open(options){const result=await dialog.showOpenDialog(window,options);return result.canceled?null:result.filePaths[0];}
async function output(options){const result=await dialog.showSaveDialog(window,options);return result.canceled?null:result.filePath;}
function setupIPC(){
handle('publish-mods',()=>service.publishMods());
handle('project',()=>service.project());
handle('edit-project',(changes,revision)=>service.editProject(changes,revision));
handle('edit-project-selection',(selected,revision)=>service.editProjectSelection(selected,revision));
handle('project-history',(direction,revision)=>service.projectHistory(direction,revision));
handle('save-project',revision=>service.saveProject(revision));
handle('apply-project',revision=>service.applyProject(revision));
handle('snapshot',()=>service.snapshot());
handle('save-mod-details',(id,details)=>service.saveModDetails(id,details));
handle('logs',()=>service.logs());
handle('choose-game',async()=>{const file=await open({title:'Choose your Rage Racer disc image',filters:[{name:'Disc images',extensions:['cue','chd','bin']}],properties:['openFile']});if(file)return service.prepare(file);return null;});
handle('cancel',()=>service.cancel());
handle('settings',values=>service.saveSettings(values));
handle('play',()=>service.launch());
handle('saves',async()=>{const list=await service.saves();savePaths=new Map(list.map((s,i)=>[String(i),s.path]));return list.map((s,i)=>({id:String(i),team:s.team,money:s.money}));});
handle('new-save',async region=>{
  const prefixes={'PAL':'BESCES-00650','NTSC-U':'BASLUS-00403','NTSC-J':'BISLPS-00600'};
  if(!Object.hasOwn(prefixes,region))throw Error('Invalid save region');
  const file=await output({title:'Create a new save',defaultPath:prefixes[region]+' RAGE000'});
  if(!file)return null;
  const data=await service.newSave(file,region);currentSave={file};
  return{name:path.basename(file),...data};
});
handle('open-save',async id=>{
  let file;if(id!==undefined){file=savePaths.get(id);if(!file)throw Error('Unknown save');}
  else file=await open({title:'Open a save or memory card',properties:['openFile']});
  if(!file)return null;const data=await service.readSave(file);currentSave={file};return{name:path.basename(file),...data};
});
handle('card-entry',async index=>{if(!currentSave||!Number.isInteger(index)||index<0||index>14)throw Error('Select a card entry');const data=await service.readSave(currentSave.file,index);currentSave.cardIndex=index;return{name:path.basename(currentSave.file),...data};});
handle('save-copy',async edits=>{if(!currentSave)throw Error('Open a save first');const file=await output({title:'Save a separate copy',defaultPath:path.basename(currentSave.file)});if(!file)return null;const data=await service.writeSave(currentSave.file,file,currentSave.cardIndex,edits);return{name:path.basename(file),...data};});
handle('assets',()=>service.assets());
handle('cars',()=>service.cars());
handle('audio',()=>service.audio());
handle('import-audio',async(id,revision)=>{require('./audio.cjs').slot(id);const file=await open({title:'Import menu music WAV',filters:[{name:'WAV audio',extensions:['wav']}],properties:['openFile']});return file?service.importAudio(id,file,revision):null;});
handle('export-audio',async id=>{require('./audio.cjs').slot(id);const file=await output({title:'Export effective music WAV',defaultPath:id+'.wav',filters:[{name:'WAV audio',extensions:['wav']}]});return file?service.exportAudio(id,file):null;});
handle('edit-audio',(id,values,revision)=>service.editAudio(id,values,revision));
handle('reset-audio',(id,revision)=>service.resetAudio(id,revision));
handle('title-images',()=>service.titleImages());
handle('import-title-image',async(id,revision)=>{require('./title-images.cjs').slot(id);const file=await open({title:'Import title menu PNG (112×16)',filters:[{name:'PNG images',extensions:['png']}],properties:['openFile']});return file?service.importTitleImage(id,file,revision):null;});
handle('export-title-image',async id=>{require('./title-images.cjs').slot(id);const file=await output({title:'Export title menu PNG',defaultPath:id+'.png',filters:[{name:'PNG images',extensions:['png']}]});return file?service.exportTitleImage(id,file):null;});
handle('reset-title-image',(id,revision)=>service.resetTitleImage(id,revision));
handle('tracks',()=>service.tracks());
handle('track-scene',bank=>service.trackScene(bank));
handle('track-textures',(id,part)=>service.trackTextures(id,part));
handle('import-track-texture',async(id,part,source,revision)=>{const file=await open({title:'Replace track surface PNG',filters:[{name:'PNG image',extensions:['png']}],properties:['openFile']});return file?service.importTrackTexture(id,part,source,file,revision):null;});
handle('export-track-texture',async(id,part,source)=>{const file=await output({title:'Export effective track surface PNG',defaultPath:'track-surface-'+source+'.png',filters:[{name:'PNG image',extensions:['png']}]});return file?service.exportTrackTexture(id,part,source,file):null;});
handle('reset-track-texture',(id,part,source,revision)=>service.resetTrackTexture(id,part,source,revision));
handle('preview-track-part',(id,part)=>service.previewTrackPart(id,part));
handle('import-track-part',async(id,part,revision)=>{const file=await open({title:'Import track model',filters:[{name:'3D model',extensions:['obj','glb','gltf']}],properties:['openFile']});return file?service.importTrackPart(id,part,file,revision):null;});
handle('export-track-part',async(id,part,format='obj')=>{if(!['obj','glb'].includes(format))throw Error('Choose OBJ or GLB export');const dir=await open({title:'Export track model '+format.toUpperCase()+' to folder',properties:['openDirectory','createDirectory']});return dir?service.exportTrackPart(id,part,dir,format):null;});
handle('import-track-mesh',async(id,revision)=>{const file=await open({title:'Import native track model bank',filters:[{name:'Native model bank',extensions:['rmesh']}],properties:['openFile']});return file?service.importTrackMesh(id,file,revision):null;});
handle('export-track-mesh',async id=>{const file=await output({title:'Export native track model bank',defaultPath:id+'.rmesh',filters:[{name:'Native model bank',extensions:['rmesh']}]});return file?service.exportTrackMesh(id,file):null;});
handle('track-preview',bank=>service.trackScene(bank,true));
handle('add-track-object',(bank,model,point,revision)=>service.addTrackObject(bank,model,point,revision));
handle('duplicate-track-object',(bank,index,revision)=>service.duplicateTrackObject(bank,index,revision));
handle('remove-track-object',(bank,index,revision)=>service.removeTrackObject(bank,index,revision));
handle('save-track-object',(bank,index,values,revision)=>service.saveTrackObject(bank,index,values,revision));
handle('save-road-point',(bank,index,values,revision)=>service.saveRoadPoint(bank,index,values,revision));
handle('save-ai-hint',(bank,index,values,revision)=>service.saveAiHint(bank,index,values,revision));
handle('save-ai-speed',(bank,index,values,revision)=>service.saveAiSpeed(bank,index,values,revision));
handle('save-start',(bank,index,values,revision)=>service.saveStart(bank,index,values,revision));
handle('save-intro',(bank,index,values,revision)=>service.saveIntro(bank,index,values,revision));
handle('save-camera',(bank,index,values,revision)=>service.saveCamera(bank,index,values,revision));
handle('save-environment',(bank,entries,revision)=>service.saveEnvironment(bank,entries,revision));
handle('save-terrain-cell',(bank,index,values,revision)=>service.saveTerrainCell(bank,index,values,revision));
handle('save-road-arc',(bank,index,values,revision)=>service.saveRoadArc(bank,index,values,revision));
handle('export-car-set',async key=>{const dir=await open({title:'Export complete car to folder',properties:['openDirectory','createDirectory']});if(dir)return service.exportCarSet(key,dir);return null;});
handle('import-car-set',async(key,format)=>{
 const file=await open(format==='glb'?{title:'Import complete car GLB or glTF',filters:[{name:'Complete car',extensions:['glb','gltf']}],properties:['openFile']}:{title:'Choose car set folder (car-set.json, models and textures)',properties:['openDirectory']});if(!file)return null;
 try{return await service.importCarSet(key,file);}catch(e){if(format==='glb'&&/Missing car part|Missing material mapping|Choose a complete GLB exported/.test(e.message))return {mapping:await service.prepareCarImport(key,file)};throw e;}
});
handle('complete-car-import',(token,mapping)=>service.completeCarImport(token,mapping));
handle('cancel-car-import',token=>service.cancelCarImport(token));
handle('preview-car-import',(token,node)=>service.previewCarImport(token,node));
handle('car-import-texture',(token,word)=>service.carImportTexture(token,word));
handle('preview-whole-car',(bank,body,rival,modId)=>service.previewWholeCar(bank,body,rival,modId));
handle('car-spec',bank=>service.carSpec(bank));
handle('save-car-spec',(bank,edits,revision)=>service.saveCarSpec(bank,edits,revision));
handle('preview-car',(key,modId)=>service.previewCar(key,modId));
handle('car-materials',key=>service.carMaterials(key));
handle('import-car',async key=>{const file=await open({title:'Import a replacement car part',filters:[{name:'Car models',extensions:['obj','glb','gltf','rmesh']}],properties:['openFile']});if(file)return service.importCar(key,file);return null;});
handle('export-car',async(key,format='obj')=>{if(!['obj','glb'].includes(format))throw Error('Choose OBJ or GLB geometry');const dir=await open({title:'Export car model to folder',properties:['openDirectory','createDirectory']});if(dir)return service.exportCar(key,dir,format);return null;});
handle('export-asset',async index=>{const file=await output({title:'Export raw asset',defaultPath:`asset_${String(index).padStart(3,'0')}.bin`});if(file)await service.exportAsset(index,file);return !!file;});
handle('replace-asset',async index=>{const file=await open({title:'Choose a replacement raw asset',filters:[{name:'Raw asset',extensions:['bin']}],properties:['openFile']});if(file)return service.replaceAsset(index,file);return null;});
handle('save-material',(id,key,properties,revision)=>service.saveMaterial(id,key,properties,revision));
handle('import-mod',async()=>{const dir=await open({title:'Import a mod folder',properties:['openDirectory']});if(dir)await service.importMod(dir);});
handle('toggle-mod',(id,enabled)=>service.toggleMod(id,enabled));
handle('resolve-conflict',(key,winner)=>service.resolveConflict(key,winner));
handle('remove-mod',async id=>{const answer=await dialog.showMessageBox(window,{type:'question',message:'Remove this mod from the launcher?',detail:'The original folder will not be modified.',buttons:['Cancel','Remove'],defaultId:0,cancelId:0});if(answer.response===1)await service.removeMod(id);});
handle('export-mod',async id=>{const dir=await open({title:'Choose export destination',properties:['openDirectory','createDirectory']});if(dir)return service.exportMod(id,dir);});
}
async function createWindow(){
  window=new BrowserWindow({show:!hidden,width:1280,height:900,minWidth:960,minHeight:680,backgroundColor:'#101319',title:'Rage Racer Editor',webPreferences:{preload:path.join(__dirname,'preload.cjs'),contextIsolation:true,sandbox:true,nodeIntegration:false,backgroundThrottling:!hidden}});
  window.webContents.setWindowOpenHandler(()=>({action:'deny'}));
  window.webContents.on('will-navigate',event=>event.preventDefault());
  window.webContents.session.setPermissionRequestHandler((_w,_p,callback)=>callback(false));
  window.webContents.on('will-prevent-unload',event=>{
    const choice=dialog.showMessageBoxSync(window,{
      type:'question',title:'Unsaved changes',message:'Discard unsaved changes and close?',
      detail:'Keep editing to save your changes first.',
      buttons:['Keep editing','Discard and close'],defaultId:0,cancelId:0,
    });
    // Electron requires preventDefault here to override the renderer veto.
    if(choice===1)event.preventDefault();
  });
  window.on('close',event=>{if(service.busy){event.preventDefault();dialog.showMessageBox(window,{message:'Wait for preparation to finish or cancel it before closing.'});}});
  await window.loadFile(page);
}
app.whenReady().then(async()=>{
  service=new LauncherService({root:app.getPath('userData'),bin:path.join(resources,'bin'),config:path.join(resources,'rage-port.ini'),onChange:state=>{if(window&&!window.isDestroyed())window.webContents.send('launcher:state',state);}});
  await service.init();setupIPC();
  Menu.setApplicationMenu(Menu.buildFromTemplate([{label:'Rage Racer Editor',submenu:[{role:'about'},{type:'separator'},{role:'quit'}]},{label:'Edit',submenu:[{role:'undo'},{role:'redo'},{type:'separator'},{role:'cut'},{role:'copy'},{role:'paste'},{role:'selectAll'}]},{label:'View',submenu:[{role:'resetZoom'},{role:'zoomIn'},{role:'zoomOut'},{role:'togglefullscreen'}]}]));
  await createWindow();app.on('activate',()=>{if(BrowserWindow.getAllWindows().length===0)createWindow();});
}).catch(e=>{dialog.showErrorBox('Launcher could not start',e.message);app.quit();});
app.on('window-all-closed',()=>{if(process.platform!=='darwin')app.quit();});
}
