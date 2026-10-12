const fs=require('node:fs/promises'),path=require('node:path');

// Reserve a new export directory atomically. Never reuse a user's previous
// export: it may contain edits that have not yet been imported into the game.
module.exports=async function exportFolder(destination,label){
 const parent=await fs.lstat(destination);
 if(!parent.isDirectory()||parent.isSymbolicLink())throw Error('Choose a normal folder for the export');
 let name=String(label).replace(/[\x00-\x1f\x7f<>:"/\\|?*]/g,' ').replace(/\s+/g,' ').replace(/^[. ]+|[. ]+$/g,'');
 const letters=Array.from(name);while(Buffer.byteLength(letters.join(''))>160)letters.pop();name=letters.join('').trim().replace(/[. ]+$/g,'')||'Unnamed';
 const base='Car - '+name;
 for(let version=1;version<=10000;version++){
  const folder=path.join(destination,base+(version===1?'':' ('+version+')'));
  try{await fs.mkdir(folder);return folder;}catch(e){if(e.code!=='EEXIST')throw e;}
 }
 throw Error('Too many exports with this name; choose another folder');
};
