const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os'),{spawn}=require('node:child_process');
const executable=process.env.RR_EDITOR_PACKAGE;
const {run}=require('../main/service.cjs');
const pause=ms=>new Promise(resolve=>setTimeout(resolve,ms));
for(const seeded of [false,true])test('packaged editor boots hidden with real preload/IPC and '+(seeded?'its private stored settings':'a clean isolated profile'), {skip:!executable,timeout:45000},async t=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-packaged-editor-'));let socket,child;
 t.after(async()=>{socket?.close();if(child&&child.exitCode===null&&child.signalCode===null){child.kill();await Promise.race([new Promise(resolve=>child.once('exit',resolve)),pause(3000)]);if(child.exitCode===null&&child.signalCode===null)child.kill('SIGKILL');}await fs.rm(root,{recursive:true,force:true});});
 if(seeded)await fs.writeFile(path.join(root,'launcher.json'),JSON.stringify({settings:{'video.internal_scale':'2'},disc:null,mods:[]}));
 const resources=process.platform==='darwin'?path.resolve(path.dirname(executable),'../Resources/resources'):path.join(path.dirname(executable),'resources/resources');
 const suffix=process.platform==='win32'?'.exe':'';
 const {listPackage}=await import('../node_modules/@electron/asar/lib/asar.js');const contents=listPackage(path.join(path.dirname(resources),'app.asar'));assert.deepEqual(contents.filter(p=>/^\/(tests|native|scripts|resources|out|node_modules)(\/|$)/.test(p)),[]);
 for(const name of ['rage-racer','rage-extract','rage-pack','rage-save-cli','rage-mod-cli','rage-mesh-obj','cars.toml','cars.ntscj.toml']){const filename=name.endsWith('.toml')?name:name+suffix;assert.deepEqual(await fs.readFile(path.join(resources,'bin',filename)),await fs.readFile(path.resolve(__dirname,'../resources/bin',filename)),filename+' is stale');}
 assert.deepEqual(await fs.readFile(path.join(resources,'rage-port.ini')),await fs.readFile(path.resolve(__dirname,'../resources/rage-port.ini')));
 for(const region of ['PAL','NTSC-U','NTSC-J']){const file=path.join(root,({'PAL':'BESCES-00650','NTSC-U':'BASLUS-00403','NTSC-J':'BISLPS-00600'}[region])+' RAGE000'),save=JSON.parse(await run(path.join(resources,'bin','rage-save-cli'+suffix),['new',file,region]));assert.equal(save.checksumsValid,true);assert.equal(save.region,region);assert.ok((await fs.stat(file)).size>0);}
 const env={...process.env};delete env.ELECTRON_RUN_AS_NODE;delete env.RAGE_LAUNCHER_USER_DATA;
 child=spawn(executable,['--hidden','--user-data-dir='+root,'--remote-debugging-port=0'],{env,stdio:['ignore','pipe','pipe'],windowsHide:true});let logs='',spawnError;child.on('error',e=>spawnError=e);child.stdout.on('data',b=>logs+=b);child.stderr.on('data',b=>logs+=b);
 let port;const deadline=Date.now()+25000;
 while(Date.now()<deadline){if(spawnError)throw spawnError;if((child.exitCode!==null||child.signalCode!==null))throw Error('Editor exited before boot: '+logs);const info=await fs.readFile(path.join(root,'DevToolsActivePort'),'utf8').catch(()=>null);if(info){port=Number(info.split('\n')[0]);break;}await pause(50);}assert.ok(port,logs);
 let page;while(Date.now()<deadline){const targets=await fetch('http://127.0.0.1:'+port+'/json/list').then(r=>r.json());page=targets.find(x=>x.type==='page'&&x.url.startsWith('file:'));if(page)break;await pause(50);}assert.ok(page,logs);
 socket=new WebSocket(page.webSocketDebuggerUrl);await new Promise((resolve,reject)=>{socket.addEventListener('open',resolve,{once:true});socket.addEventListener('error',reject,{once:true});});let sequence=0;const pending=new Map();socket.addEventListener('message',e=>{const result=JSON.parse(e.data),entry=pending.get(result.id);if(!entry)return;pending.delete(result.id);clearTimeout(entry.timer);result.error?entry.reject(Error(result.error.message)):entry.resolve(result.result);});
 const call=(method,params={})=>new Promise((resolve,reject)=>{const id=++sequence,timer=setTimeout(()=>{pending.delete(id);reject(Error('CDP timed out: '+method));},10000);pending.set(id,{resolve,reject,timer});socket.send(JSON.stringify({id,method,params}));});
 async function evaluate(expression){const r=await call('Runtime.evaluate',{expression,awaitPromise:true,returnByValue:true});if(r.exceptionDetails)throw Error(JSON.stringify(r.exceptionDetails));return r.result.value;}
 let ready=false;while(Date.now()<deadline){ready=await evaluate('Boolean(window.launcher && document.querySelector("[data-page=saves]") && !document.querySelector("[data-page=saves]").disabled)');if(ready)break;await pause(50);}assert.ok(ready,logs);
 const snapshot=await evaluate('window.launcher.snapshot()');assert.equal(snapshot.ok,true);assert.equal(snapshot.value.disc,null);assert.equal(snapshot.value.project,null);assert.deepEqual(snapshot.value.mods,[]);assert.equal(snapshot.value.toolsReady,true);assert.equal(snapshot.value.error,null);assert.equal(snapshot.value.settings['video.internal_scale'],seeded?'2':'4');
 const methods=await evaluate("['trackTextures','importTrackTexture','exportTrackTexture','resetTrackTexture','saveIntro','saveCamera','saveMaterial'].map(name=>typeof window.launcher[name])");assert.deepEqual(methods,Array(7).fill('function'));
 await evaluate('document.querySelector("[data-page=saves]").click()');let text='';while(Date.now()<deadline){text=await evaluate('document.querySelector("#main").innerText');if(text.includes('Save editor'))break;await pause(50);}assert.match(text,/Save editor/);assert.match(text,/Open|Create/);
 const noSource=await evaluate('window.launcher.trackTextures("track.big1.course",0)');assert.equal(noSource.ok,false);assert.match(noSource.error,/disc|prepare|game|source/i);
 assert.equal(await evaluate('document.querySelector("[data-page=settings]").disabled'),true);
 // Verify Electron service storage follows the requested root, not the user profile.
 const files=await fs.readdir(root);assert.ok(files.includes('DevToolsActivePort'));if(seeded)assert.ok(files.includes('launcher.json'));
 assert.doesNotMatch(logs,/Unable to load preload|Cannot find module|Launcher could not start/i);
 const capture=process.env.RR_EDITOR_PACKAGE_CAPTURE;if(capture){const frame=await call('Page.captureScreenshot',{format:'png'});await fs.mkdir(path.dirname(capture),{recursive:true});await fs.writeFile(capture,Buffer.from(frame.data,'base64'));}
});
