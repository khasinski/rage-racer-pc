const fs=require('node:fs/promises'),path=require('node:path');

// Generated PCM belongs to the imported library. Repair it with the normal
// extractor, but publish only this file after successful, uncancelled decoding.
module.exports=async function menuMusic(service,signal){
 const disc=service.state.disc,file=path.join(disc.data,'menu_music.wav');
 const existing=await fs.lstat(file).catch(e=>{if(e.code==='ENOENT')return null;throw e;});
 if(existing){if(!existing.isFile()||existing.isSymbolicLink())throw Error('Menu music cache is not a regular file');return file;}
 if(signal?.aborted)throw Error('Operation canceled');
 if(!disc.archive)throw Error('Reimport the game image to restore missing menu music');
 const {run,atomic,source}=require('./service.cjs'),identity=await source(disc);
 const temp=await fs.mkdtemp(path.join(service.root,'music-repair-'));
 try{
  await run(service.tool('rage-extract'),[disc.archive,path.join(temp,'data')],{cwd:temp,signal});
  const generated=path.join(temp,'data/menu_music.wav');
  await run(service.tool('rage-mod-cli'),['--wav-info',generated],{signal});
  if((await source(disc)).fingerprint!==identity.fingerprint)throw Error('Source archive changed while generating music; reimport the game image');
  const bytes=await fs.readFile(generated);
  if(signal?.aborted)throw Error('Operation canceled');
  await atomic(file,bytes);
  return file;
 }finally{await fs.rm(temp,{recursive:true,force:true});}
};
