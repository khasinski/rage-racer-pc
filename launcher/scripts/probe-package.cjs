// Exercise the actual packaged, sandboxed UI with an empty profile. No disc is
// needed for this probe; disc preparation requires a separate owned-data test.
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {spawn,execFileSync}=require('node:child_process');
(async()=>{
 const platform=process.platform,root=path.resolve(__dirname,'..');
 const directory=path.join(root,'out',`Rage Mod Manager-${platform}-${process.arch}`);
 const executable=platform==='darwin'?path.join(directory,'Rage Mod Manager.app','Contents','MacOS','rage-launcher'):path.join(directory,'rage-launcher'+(platform==='win32'?'.exe':''));
 const profile=await fs.mkdtemp(path.join(os.tmpdir(),'rage-packaged-probe-'));
 const env={...process.env};delete env.ELECTRON_RUN_AS_NODE;
 let child,ws,timer;const pending=new Map();let next=0,diagnostics='';
 try{
  const deadline=new Promise((_,reject)=>{timer=setTimeout(()=>reject(Error('Packaged startup probe timed out\n'+diagnostics.slice(-4000))),30000);});
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
    const reply=await call('Runtime.evaluate',{expression:`(async()=>{if(!window.launcher||!document.querySelector('[data-action="choose-game"]'))return null;const s=await window.launcher.snapshot();return {snapshot:s,lang:document.documentElement.lang,locked:[...document.querySelectorAll('#nav [data-page]')].filter(b=>!['home','source'].includes(b.dataset.page)).every(b=>b.disabled)};})()`,awaitPromise:true,returnByValue:true});
    if(reply.exceptionDetails)throw Error(JSON.stringify(reply.exceptionDetails));result=reply.result?.value;if(result)break;await new Promise(r=>setTimeout(r,100));
   }
   if(!result?.snapshot.ok||result.snapshot.value.ready||!result.snapshot.value.toolsReady||!result.locked||result.lang!=='en')throw Error('Invalid packaged first-run state: '+JSON.stringify(result));
   console.log('Packaged first-run UI passed on '+platform+'/'+process.arch);
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
