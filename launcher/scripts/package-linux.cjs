const fs=require('node:fs/promises'),path=require('node:path'),{spawn}=require('node:child_process');
// Debian installation supplies root ownership for Chromium's sandbox helper.
// The unpacked development directory is not a portable sandboxed Linux install.
module.exports=async function packageLinux(bundle,out,version){
 const arch={x64:'amd64',arm64:'arm64'}[process.arch];if(!arch)throw Error('Unsupported Debian architecture');
 if(!/^\d+\.\d+\.\d+(?:[-+][a-zA-Z0-9.]+)?$/.test(version))throw Error('Invalid package version');
 const stage=await fs.mkdtemp(path.join(out,'.deb-')),application=path.join(stage,'opt/rage-racer-editor'),target=path.join(out,`Rage-Racer-Editor-${version}-${arch}.deb`);
 try{
  await fs.chmod(stage,0o755);
  await fs.cp(bundle,application,{recursive:true});await fs.chmod(application,0o755);const helper=await fs.lstat(path.join(application,'chrome-sandbox'));if(!helper.isFile()||helper.isSymbolicLink())throw Error('Missing regular Chromium sandbox helper');await fs.chmod(path.join(application,'chrome-sandbox'),0o4755);
  await fs.mkdir(path.join(stage,'DEBIAN'));await fs.writeFile(path.join(stage,'DEBIAN/control'),`Package: rage-racer-editor
Version: ${version}
Architecture: ${arch}
Maintainer: Rage Racer contributors <maintainers@rageracer.invalid>
Depends: libc6, libstdc++6, libgcc-s1, libdbus-1-3, libvulkan1, libgtk-3-0 | libgtk-3-0t64, libnss3, libasound2 | libasound2t64, libgbm1, libx11-6, libxcomposite1, libxdamage1, libxrandr2, libxss1, libxtst6
Section: games
Priority: optional
Description: Rage Racer game manager and asset editor
 Unified local editor with bundled native game and asset tools.
`);
  const desktop=path.join(stage,'usr/share/applications');await fs.mkdir(desktop,{recursive:true});await fs.writeFile(path.join(desktop,'rage-racer-editor.desktop'),`[Desktop Entry]
Type=Application
Name=Rage Racer Editor
Comment=Manage Rage Racer and edit cars, tracks and saves
Exec=/opt/rage-racer-editor/rage-launcher
Icon=applications-games
Terminal=false
Categories=Game;Utility;
`);
  await new Promise((resolve,reject)=>{const child=spawn('dpkg-deb',['--root-owner-group','--build',stage,target],{stdio:'inherit'});child.on('error',reject);child.on('exit',code=>code===0?resolve():reject(Error('Debian packaging failed: '+code)));});return target;
 }finally{await fs.rm(stage,{recursive:true,force:true});}
};
