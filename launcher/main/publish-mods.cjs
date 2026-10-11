const fs=require('node:fs/promises'),path=require('node:path'),{randomUUID}=require('node:crypto');
// Publish one composed selection. Imported projects remain separate and can
// still be disabled, edited or exported; the running game sees one stable tree.
module.exports=async function publishMods(service){
 service.requireReady();if(service.game)throw Error('Close the game before saving its mods');
 return service.operation('Saving enabled mods for the game',async()=>{
  const {run}=require('./service.cjs');
  const {directory}=JSON.parse(await run(service.tool('rage-mod-cli'),['--mod-directory']));
  if(typeof directory!=='string'||!path.isAbsolute(directory))throw Error('The game mod directory is unavailable');
  const marker=path.join(directory,'.rage-editor.json');
  const existing=await fs.lstat(directory).catch(e=>{if(e.code==='ENOENT')return null;throw e;});
  if(existing){
   if(existing.isSymbolicLink()||!existing.isDirectory())throw Error('The game mod path is not a normal directory');
   const owner=JSON.parse(await fs.readFile(marker,'utf8').catch(()=>'{"format":0}'));
   if(owner.format!==1||owner.owner!=='rage-mod-manager')throw Error('This directory contains a manually installed mod. Import it into My mods before replacing it.');
  }
  const composed=await service.composeMods();
  if(!composed)throw Error('Enable at least one mod before saving for the game');
  const token=randomUUID(),staging=directory+'.staging-'+token,backup=directory+'.previous-'+token;
  let moved=false,published=false;
  try{
   await fs.mkdir(path.dirname(directory),{recursive:true});
   const files=await require('./mods.cjs').inventory(composed,service.tool('rage-mod-cli'));
   await service.snapshotModFiles(composed,staging,files);
   await fs.writeFile(path.join(staging,'.rage-editor.json'),JSON.stringify({format:1,owner:'rage-mod-manager',region:service.state.disc.region}));
   if(existing){await fs.rename(directory,backup);moved=true;}
   try{await fs.rename(staging,directory);published=true;}catch(e){if(moved){await fs.rename(backup,directory);moved=false;}throw e;}
   return directory;
  }finally{
   await fs.rm(composed,{recursive:true,force:true});
   await fs.rm(staging,{recursive:true,force:true});
   if(published&&moved)await fs.rm(backup,{recursive:true,force:true});
  }
 },false);
};
