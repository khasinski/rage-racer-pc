const {spawn}=require('node:child_process');
const path=require('node:path');
const env={...process.env};delete env.ELECTRON_RUN_AS_NODE;
const child=spawn(require('electron'),[path.resolve(__dirname,'..')],{stdio:'inherit',env,shell:false});
child.on('error',e=>{console.error(e);process.exit(1);});
child.on('exit',code=>process.exit(code||0));
