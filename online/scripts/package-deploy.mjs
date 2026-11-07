// Backend-only update over a verified current image. Browser assets and disc
// remain in the base, and persistent SQLite stays on the existing /data volume.
import {parseArgs} from 'node:util';
import {cpSync,existsSync,mkdtempSync,readdirSync,rmSync,writeFileSync} from 'node:fs';
import {join,resolve} from 'node:path';
import {tmpdir} from 'node:os';
import {fileURLToPath} from 'node:url';
import {spawnSync} from 'node:child_process';
const {values}=parseArgs({options:{'base-image':{type:'string'},out:{type:'string'},build:{type:'string'}}});
const base=values['base-image'],out=values.out&&resolve(values.out),build=values.build;
if(!base||!out||!build||!/^[-a-zA-Z0-9_./:@]+$/.test(base)||!/^[-a-zA-Z0-9_.]+$/.test(build))throw new Error('usage: package-deploy.mjs --base-image IMAGE --out FILE.tar --build COMMIT');
const online=fileURLToPath(new URL('..',import.meta.url));
if(!existsSync(join(online,'server/wasm/rage-server.wasm')))throw new Error('Build the server simulation first');
const stage=mkdtempSync(join(tmpdir(),'rage-backend-'));
try{
  cpSync(join(online,'shared'),join(stage,'shared'),{recursive:true});
  cpSync(join(online,'server/wasm'),join(stage,'server/wasm'),{recursive:true});
  for(const file of readdirSync(join(online,'server')).filter(f=>f.endsWith('.ts')&&!f.endsWith('.test.ts')))cpSync(join(online,'server',file),join(stage,'server',file));
  writeFileSync(join(stage,'captain-definition'),JSON.stringify({schemaVersion:2,dockerfilePath:'./Dockerfile'}));
  writeFileSync(join(stage,'Dockerfile'),`FROM ${base}\nCOPY shared /app/shared\nCOPY server /app/server\nENV RAGE_BUILD_ID=${build}\nHEALTHCHECK --interval=30s --timeout=5s --start-period=20s CMD node -e "fetch('http://127.0.0.1:7243/healthz').then(r=>process.exit(r.ok?0:1)).catch(()=>process.exit(1))"\n`);
  const result=spawnSync('tar',[...(process.platform==='darwin'?['--no-xattrs']:[]),'-cf',out,'-C',stage,'.'],{stdio:'inherit',env:{...process.env,COPYFILE_DISABLE:'1'}});if(result.status!==0)process.exitCode=result.status??1;
  else console.log(`Backend bundle: ${out}`);
}finally{rmSync(stage,{recursive:true,force:true});}
