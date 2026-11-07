import {spawnSync} from 'node:child_process';
import {mkdirSync,copyFileSync} from 'node:fs';
import {fileURLToPath} from 'node:url';
import {resolve} from 'node:path';
const root=fileURLToPath(new URL('../..',import.meta.url));
const build=resolve(root,'build/online-server');
for(const [cmd,args] of [
 ['emcmake',['cmake','-S',resolve(root,'src/online/runtime'),'-B',build,'-DCMAKE_BUILD_TYPE=Release']],
 ['cmake',['--build',build,'--target','rage-server','--parallel','4']],
]){
 const result=spawnSync(cmd,args,{cwd:root,stdio:'inherit',shell:false});
 if(result.status!==0)process.exit(result.status??1);
}
const out=resolve(root,'online/server/wasm');mkdirSync(out,{recursive:true});
for(const ext of ['mjs','wasm'])copyFileSync(resolve(build,`rage-server.${ext}`),resolve(out,`rage-server.${ext}`));
