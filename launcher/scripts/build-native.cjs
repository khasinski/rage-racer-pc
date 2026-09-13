const fs = require('node:fs');
const path = require('node:path');
const { spawnSync } = require('node:child_process');
const root=path.resolve(__dirname,'../..');
const build=process.env.RAGE_LAUNCHER_BUILD_DIR||path.join(root,'build','release');
function run(args){const r=spawnSync('cmake',args,{cwd:root,stdio:'inherit',shell:false});if(r.status!==0)process.exit(r.status||1);}
const generator=process.platform==='win32'?['-G','Visual Studio 17 2022','-A','x64','-T','ClangCL']:[];
run(['-S',root,'-B',build,...generator,'-DCMAKE_BUILD_TYPE=Release','-DBUILD_TESTING=ON']);
run(['--build',build,'--config','Release','--parallel','6','--target','rage-save-format-tests','rage-save-generator','rage-racer','rage-extract','rage-pack','rage-save-cli','rage-mod-cli','rage-mesh-obj']);
const bin=path.join(root,'launcher','resources','bin');fs.mkdirSync(bin,{recursive:true});
const ext=process.platform==='win32'?'.exe':'';
for(const name of ['rage-racer','rage-extract','rage-pack','rage-save-cli','rage-mod-cli','rage-mesh-obj']){
  const candidates=[path.join(build,name+ext),path.join(build,'Release',name+ext)];
  if(name==='rage-racer'&&process.platform==='darwin')candidates.unshift(path.join(build,'Rage Racer.app','Contents','MacOS','Rage Racer'));
  const source=candidates.find(fs.existsSync);if(!source)throw Error('Missing native tool: '+name);
  fs.copyFileSync(source,path.join(bin,name+ext));if(process.platform!=='win32')fs.chmodSync(path.join(bin,name),0o755);
}
// Windows runtime libraries beside the game must travel with the launcher.
if(process.platform==='win32')for(const dir of [build,path.join(build,'Release')])if(fs.existsSync(dir))for(const name of fs.readdirSync(dir))if(name.toLowerCase().endsWith('.dll'))fs.copyFileSync(path.join(dir,name),path.join(bin,name));
// resources/rage-port.ini is the launcher's checked-in distribution template.
// The repository-root INI may contain a developer's local paths or diagnostics.
fs.accessSync(path.join(root,'launcher','resources','rage-port.ini'),fs.constants.R_OK);
console.log('Native runtime staged in '+bin);
