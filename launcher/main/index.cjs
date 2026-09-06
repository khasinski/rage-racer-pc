const {app,BrowserWindow,dialog,ipcMain,Menu}=require('electron');
const path=require('node:path');
const {pathToFileURL}=require('node:url');
const {LauncherService}=require('./service.cjs');
let window,service,savePaths=new Map(),currentSave;
const page=path.join(__dirname,'../renderer/index.html');
const resources=app.isPackaged?path.join(process.resourcesPath,'resources'):path.join(__dirname,'../resources');
const testRoot=process.env.RAGE_LAUNCHER_USER_DATA;
if(testRoot&&!app.isPackaged)app.setPath('userData',testRoot);
else if(!app.commandLine.hasSwitch('user-data-dir')){
  // Keep existing profiles when changing the visible product name.
  app.setPath('userData',path.join(app.getPath('appData'),'Rage Racer Launcher'));
}
if(!app.requestSingleInstanceLock())app.quit();
else {
app.on('second-instance',()=>{window?.show();window?.focus();});
function handle(name,fn){ipcMain.handle('launcher:'+name,async(event,...args)=>{
  if(event.sender!==window?.webContents||event.senderFrame!==window.webContents.mainFrame||event.senderFrame.url!==pathToFileURL(page).href)throw Error('Untrusted request');
  try{return{ok:true,value:await fn(...args)};}catch(e){return{ok:false,error:e.message};}
});}
async function open(options){const result=await dialog.showOpenDialog(window,options);return result.canceled?null:result.filePaths[0];}
async function output(options){const result=await dialog.showSaveDialog(window,options);return result.canceled?null:result.filePath;}
function setupIPC(){
handle('snapshot',()=>service.snapshot());
handle('save-mod-details',(id,details)=>service.saveModDetails(id,details));
handle('logs',()=>service.logs());
handle('choose-game',async()=>{const file=await open({title:'Choose your Rage Racer disc image',filters:[{name:'Disc images',extensions:['cue','chd','bin']}],properties:['openFile']});if(file)return service.prepare(file);return null;});
handle('cancel',()=>service.cancel());
handle('settings',values=>service.saveSettings(values));
handle('play',()=>service.launch());
handle('saves',async()=>{const list=await service.saves();savePaths=new Map(list.map((s,i)=>[String(i),s.path]));return list.map((s,i)=>({id:String(i),team:s.team,money:s.money}));});
handle('open-save',async id=>{
  let file;if(id!==undefined){file=savePaths.get(id);if(!file)throw Error('Unknown save');}
  else file=await open({title:'Open a save or memory card',properties:['openFile']});
  if(!file)return null;const data=await service.readSave(file);currentSave={file};return{name:path.basename(file),...data};
});
handle('card-entry',async index=>{if(!currentSave||!Number.isInteger(index)||index<0||index>14)throw Error('Select a card entry');const data=await service.readSave(currentSave.file,index);currentSave.cardIndex=index;return{name:path.basename(currentSave.file),...data};});
handle('save-copy',async edits=>{if(!currentSave)throw Error('Open a save first');const file=await output({title:'Save a separate copy',defaultPath:path.basename(currentSave.file)});if(!file)return null;const data=await service.writeSave(currentSave.file,file,currentSave.cardIndex,edits);return{name:path.basename(file),...data};});
handle('assets',()=>service.assets());
handle('cars',()=>service.cars());
handle('export-car-set',async key=>{const dir=await open({title:'Export complete car to folder',properties:['openDirectory','createDirectory']});if(dir)return service.exportCarSet(key,dir);return null;});
handle('import-car-set',async key=>{const dir=await open({title:'Choose car set folder (car-set.json, models and textures)',properties:['openDirectory']});if(dir)return service.importCarSet(key,dir);return null;});
handle('preview-whole-car',(bank,body,rival,modId)=>service.previewWholeCar(bank,body,rival,modId));
handle('car-spec',bank=>service.carSpec(bank));
handle('save-car-spec',(bank,edits)=>service.saveCarSpec(bank,edits));
handle('preview-car',(key,modId)=>service.previewCar(key,modId));
handle('import-car',async key=>{const file=await open({title:'Import a replacement car part',filters:[{name:'Car models',extensions:['obj','rmesh']}],properties:['openFile']});if(file)return service.importCar(key,file);return null;});
handle('export-car',async key=>{const dir=await open({title:'Export car model to folder',properties:['openDirectory','createDirectory']});if(dir)return service.exportCar(key,dir);return null;});
handle('export-asset',async index=>{const file=await output({title:'Export raw asset',defaultPath:`asset_${String(index).padStart(3,'0')}.bin`});if(file)await service.exportAsset(index,file);return !!file;});
handle('replace-asset',async index=>{const file=await open({title:'Choose a replacement raw asset',filters:[{name:'Raw asset',extensions:['bin']}],properties:['openFile']});if(file)return service.replaceAsset(index,file);return null;});
handle('save-material',(id,key,properties)=>service.saveMaterial(id,key,properties));
handle('import-mod',async()=>{const dir=await open({title:'Import a mod folder',properties:['openDirectory']});if(dir)await service.importMod(dir);});
handle('toggle-mod',(id,enabled)=>service.toggleMod(id,enabled));
handle('resolve-conflict',(key,winner)=>service.resolveConflict(key,winner));
handle('remove-mod',async id=>{const answer=await dialog.showMessageBox(window,{type:'question',message:'Remove this mod from the launcher?',detail:'The original folder will not be modified.',buttons:['Cancel','Remove'],defaultId:0,cancelId:0});if(answer.response===1)await service.removeMod(id);});
handle('export-mod',async id=>{const dir=await open({title:'Choose export destination',properties:['openDirectory','createDirectory']});if(dir)return service.exportMod(id,dir);});
}
async function createWindow(){
  window=new BrowserWindow({width:1280,height:900,minWidth:960,minHeight:680,backgroundColor:'#101319',title:'Rage Mod Manager',webPreferences:{preload:path.join(__dirname,'preload.cjs'),contextIsolation:true,sandbox:true,nodeIntegration:false}});
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
  Menu.setApplicationMenu(Menu.buildFromTemplate([{label:'Rage Mod Manager',submenu:[{role:'about'},{type:'separator'},{role:'quit'}]},{label:'Edit',submenu:[{role:'undo'},{role:'redo'},{type:'separator'},{role:'cut'},{role:'copy'},{role:'paste'},{role:'selectAll'}]},{label:'View',submenu:[{role:'resetZoom'},{role:'zoomIn'},{role:'zoomOut'},{role:'togglefullscreen'}]}]));
  await createWindow();app.on('activate',()=>{if(BrowserWindow.getAllWindows().length===0)createWindow();});
}).catch(e=>{dialog.showErrorBox('Launcher could not start',e.message);app.quit();});
app.on('window-all-closed',()=>{if(process.platform!=='darwin')app.quit();});
}
