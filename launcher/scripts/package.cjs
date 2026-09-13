const path=require('node:path');
const fs=require('node:fs');
(async()=>{
  const root=path.resolve(__dirname,'..');
  const extension=process.platform==='win32'?'.exe':'';
  for(const name of ['rage-racer','rage-extract','rage-pack','rage-save-cli','rage-mod-cli','rage-mesh-obj']){
    const file=path.join(root,'resources','bin',name+extension);
    if(!fs.existsSync(file)||!fs.statSync(file).isFile())throw Error(`Missing ${name}. Run npm run build:native before packaging`);
    fs.accessSync(file,process.platform==='win32'?fs.constants.R_OK:fs.constants.R_OK|fs.constants.X_OK);
  }
  fs.accessSync(path.join(root,'resources','rage-port.ini'),fs.constants.R_OK);
  const {packager}=await import('@electron/packager');
  const result=await packager({dir:root,out:path.join(root,'out'),name:'Rage Mod Manager',executableName:'rage-launcher',appBundleId:'org.rageracer.launcher',overwrite:true,asar:true,prune:true,extraResource:[path.join(root,'resources')],ignore:[/^\/out($|\/)/,/^\/resources($|\/)/,/^\/tests($|\/)/,/^\/native($|\/)/,/^\/scripts($|\/)/],osxSign:undefined});
  console.log(result.join('\n'));
})().catch(e=>{console.error(e);process.exit(1)});
