// Exercise the packaged sandboxed UI with an isolated profile. Optional
// RAGE_LAUNCHER_PROBE_CUE prepares it using packaged tools, then tests IPC Play;
// native file chooser automation is intentionally not bypassed in the app.
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {spawn,execFileSync}=require('node:child_process');
(async()=>{
 const platform=process.platform,root=path.resolve(__dirname,'..');
 const directory=path.join(root,'out',`Rage Mod Manager-${platform}-${process.arch}`);
 const executable=platform==='darwin'?path.join(directory,'Rage Mod Manager.app','Contents','MacOS','rage-launcher'):path.join(directory,'rage-launcher'+(platform==='win32'?'.exe':''));
 const profile=await fs.mkdtemp(path.join(os.tmpdir(),'rage-packaged-probe-'));
 const env={...process.env};delete env.ELECTRON_RUN_AS_NODE;
 const disc=process.env.RAGE_LAUNCHER_PROBE_CUE;
 let child,ws,timer;const pending=new Map();let next=0,diagnostics='';
 try{
  if(disc){
   const resources=platform==='darwin'?path.join(directory,'Rage Mod Manager.app','Contents','Resources','resources'):path.join(directory,'resources','resources');
   const {LauncherService}=require('../main/service.cjs');
   const service=new LauncherService({root:profile,bin:path.join(resources,'bin'),config:path.join(resources,'rage-port.ini')});
   await service.init();await service.prepare(path.resolve(disc));
   env.RAGE_TEST_SCENARIO=path.resolve(root,'../race-scenario.ini');
   env.RAGE_PORT_SMOKE_STOP_SCENE='12';env.RAGE_PORT_SMOKE_STOP_SCENE_TIMER='20';
   env.SDL_VIDEODRIVER='offscreen';env.SDL_AUDIODRIVER='dummy';env.XDG_STATE_HOME=path.join(profile,'state');
   delete env.RAGE_PORT_MODERN_ASSETS;
  }
  const deadline=new Promise((_,reject)=>{timer=setTimeout(()=>reject(Error('Packaged startup probe timed out\n'+diagnostics.slice(-4000))),disc?90000:30000);});
  const work=async()=>{
   const endpoint=await new Promise((resolve,reject)=>{
    const graphics=process.argv.includes('--software-rendering')?['--use-angle=swiftshader','--enable-unsafe-swiftshader']:[];
    child=spawn(executable,['--user-data-dir='+profile,'--remote-debugging-port=0',...graphics],{env,stdio:['ignore','pipe','pipe'],windowsHide:true,detached:platform!=='win32'});
    child.once('error',reject);child.once('close',(code,signal)=>reject(Error('Launcher exited before probe: '+(signal||code)+'\n'+diagnostics.slice(-4000))));
    const output=bytes=>{diagnostics+=bytes;const match=diagnostics.match(/DevTools listening on (ws:\/\/[^\s]+)/);if(match)resolve(match[1]);};
    child.stdout.on('data',output);child.stderr.on('data',output);
   });
   const origin=new URL(endpoint);const base='http://'+origin.host;let pages=[];
   for(let i=0;i<100;i++){pages=await(await fetch(base+'/json/list')).json();if(pages.some(p=>p.type==='page'&&p.url.includes('app.asar')))break;await new Promise(r=>setTimeout(r,100));}
   const page=pages.find(p=>p.type==='page'&&p.url.includes('app.asar'));if(!page)throw Error('Packaged UI did not load');
   ws=new WebSocket(page.webSocketDebuggerUrl);await new Promise((resolve,reject)=>{ws.onopen=resolve;ws.onerror=reject;});
   ws.onmessage=e=>{const message=JSON.parse(e.data);const job=pending.get(message.id);if(job){pending.delete(message.id);message.error?job.reject(Error(message.error.message)):job.resolve(message.result);}};
   const call=(method,params)=>new Promise((resolve,reject)=>{const id=++next;pending.set(id,{resolve,reject});ws.send(JSON.stringify({id,method,params}));});
   let result;
   for(let i=0;i<100;i++){
    const reply=await call('Runtime.evaluate',{expression:`(async()=>{if(!window.launcher||!document.querySelector('[data-action="choose-game"], [data-action="play"]'))return null;const s=await window.launcher.snapshot();return {snapshot:s,lang:document.documentElement.lang,locked:[...document.querySelectorAll('#nav [data-page]')].filter(b=>!['home','source'].includes(b.dataset.page)).every(b=>b.disabled)};})()`,awaitPromise:true,returnByValue:true});
    if(reply.exceptionDetails)throw Error(JSON.stringify(reply.exceptionDetails));result=reply.result?.value;if(result)break;await new Promise(r=>setTimeout(r,100));
   }
   if(!result?.snapshot.ok||result.snapshot.value.ready!==Boolean(disc)||!result.snapshot.value.toolsReady||(!disc&&!result.locked)||result.lang!=='en')throw Error('Invalid packaged first-run state: '+JSON.stringify(result));
   console.log('Packaged first-run UI passed on '+platform+'/'+process.arch);
   if(disc){
    const play=await call('Runtime.evaluate',{expression:'window.launcher.play()',awaitPromise:true,returnByValue:true});
    if(play.exceptionDetails||!play.result?.value?.ok)throw Error('Packaged Play failed: '+JSON.stringify(play));
    let finished=false;
    for(let i=0;i<600;i++){
     const reply=await call('Runtime.evaluate',{expression:'window.launcher.snapshot()',awaitPromise:true,returnByValue:true});
     const state=reply.result?.value;
     if(reply.exceptionDetails||!state?.ok)throw Error('Packaged game state unavailable');
     if(state.value.error)throw Error(state.value.error);
     if(!state.value.running&&!state.value.busy){finished=true;break;}
     await new Promise(r=>setTimeout(r,100));
    }
    if(!finished)throw Error('Packaged game did not finish');
    const log=await fs.readFile(path.join(profile,'game.log'),'utf8');
    if(!log.includes('native assets generated by C importer')||!log.includes('native GPU pipeline ready')||!log.includes('stopping at scene 12'))throw Error('Packaged Play did not reach modern race scene');
    console.log('Packaged IPC Play and modern scene startup passed');
   }
   if(process.argv.includes('--preview-regression')){
    const probe=require('./probe-preview.cjs');
    const reply=await call('Runtime.evaluate',{expression:'('+probe.toString()+')()',awaitPromise:true,returnByValue:true});
    if(reply.exceptionDetails||reply.result?.value!==true)throw Error('Packaged preview regression failed: '+JSON.stringify(reply.exceptionDetails||reply.result));
    console.log('Packaged preview comparison and close/decode regression passed');
   }
   // Let native window creation settle; quitting immediately after DOM load
   // can miss platform teardown failures that appear once the window is live.
   await new Promise(r=>setTimeout(r,3000));
   // On macOS exercise Cocoa termination: Browser.close can bypass the native
   // teardown hang seen on some Tahoe hosts. Target only this isolated PID.
   // Elsewhere the CDP transport can close before replying to Browser.close.
   if(platform==='darwin'){
    const accepted=execFileSync('osascript',['-l','JavaScript','-e',"ObjC.import('AppKit'); $.NSRunningApplication.runningApplicationWithProcessIdentifier("+child.pid+").terminate"],{encoding:'utf8',timeout:5000}).trim();
    if(accepted!=='true')throw Error('Cocoa rejected the packaged shutdown request: '+accepted);
   }else ws.send(JSON.stringify({id:++next,method:'Browser.close',params:{}}));
   for(let i=0;i<100&&child.exitCode===null&&child.signalCode===null;i++)await new Promise(r=>setTimeout(r,100));
   if(child.exitCode!==0)throw Error('Packaged application did not exit cleanly after shutdown request (code '+child.exitCode+', signal '+child.signalCode+')');
   console.log('Packaged clean shutdown passed on '+platform+'/'+process.arch);
  };
  await Promise.race([work(),deadline]);
 }finally{
  clearTimeout(timer);ws?.close();
  if(child?.pid){
   // Electron helpers inherit pipes. Stop the isolated process group, otherwise
   // an orphaned helper can keep the parent's close event pending indefinitely.
   const stopped=child.exitCode!==null||child.signalCode!==null?Promise.resolve():new Promise(r=>child.once('exit',r));
   const stop=signal=>{try{if(platform==='win32')execFileSync('taskkill',['/pid',String(child.pid),'/T','/F'],{stdio:'ignore'});else process.kill(-child.pid,signal);}catch(e){if(platform!=='win32'&&e.code!=='ESRCH')throw e;}};
   stop('SIGTERM');const force=setTimeout(()=>stop('SIGKILL'),5000);
   try{await stopped;}finally{clearTimeout(force);stop('SIGKILL');child.stdout.destroy();child.stderr.destroy();}
  }
  await fs.rm(profile,{recursive:true,force:true,maxRetries:5,retryDelay:200});
 }
})().catch(e=>{console.error(e);process.exitCode=1});
