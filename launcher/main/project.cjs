const fs=require('node:fs/promises');
const {randomUUID}=require('node:crypto');
const path=require('node:path');

const MAX_CHANGES=10000,MAX_HISTORY=200;
const kinds=new Set(['car','track','interface','audio']);
const safeName=s=>typeof s==='string'&&/^[a-z][a-z0-9_./-]{0,159}$/.test(s)&&!s.split(/[./]/).some(x=>['__proto__','prototype','constructor'].includes(x));
function checkSource(source){
  if(!source||!['PAL','NTSC-U','NTSC-J'].includes(source.region)||!/^sha256:[a-f0-9]{64}$/.test(source.fingerprint))throw Error('Invalid project source');
}
function checkChange(c){
  if(!c||!kinds.has(c.kind)||!safeName(c.id)||!safeName(c.field)||
     !(c.value===null||typeof c.value==='boolean'||typeof c.value==='string'&&c.value.length<=4096||typeof c.value==='number'&&Number.isFinite(c.value)))throw Error('Invalid project change');
}
const key=c=>`${c.kind}:${c.id}:${c.field}`;
const content=d=>JSON.stringify(d.changes);

// One history for edits across every screen. Binary parsing and gameplay
// validation belong to the compiled codecs, not to this document container.
class Project {
  constructor(source,name='My project'){
    checkSource(source);
    if(typeof name!=='string'||!name.trim()||name.length>120)throw Error('Invalid project name');
    this.document={format:1,id:randomUUID(),name,source:{region:source.region,fingerprint:source.fingerprint},changes:[]};
    this.revision=0;this.undoStack=[];this.redoStack=[];this.saved=content(this.document);
  }
  static async open(file,source){
    checkSource(source);
    const handle=await fs.open(file,'r');let d;
    try{if((await handle.stat()).size>8*1024*1024)throw Error('Project exceeds size limit');d=JSON.parse(await handle.readFile('utf8'));}finally{await handle.close();}
    if(d.format!==1||typeof d.id!=='string'||!/^[a-f0-9-]{36}$/.test(d.id)||typeof d.name!=='string'||d.name.length>120||!Array.isArray(d.changes)||d.changes.length>MAX_CHANGES)throw Error('Invalid project document');
    checkSource(d.source);
    if(d.source.region!==source.region||d.source.fingerprint!==source.fingerprint)throw Error('Project belongs to a different disc revision');
    const seen=new Set();for(const c of d.changes){checkChange(c);if(c.value===null)throw Error('Reset entries must not be stored');if(seen.has(key(c)))throw Error('Duplicate project change');seen.add(key(c));}
    const p=new Project(source,d.name);p.document={format:1,id:d.id,name:d.name,source:p.document.source,changes:d.changes.map(c=>({kind:c.kind,id:c.id,field:c.field,value:c.value}))};p.saved=content(p.document);return p;
  }
  snapshot(){return structuredClone({...this.document,revision:this.revision,dirty:content(this.document)!==this.saved,canUndo:!!this.undoStack.length,canRedo:!!this.redoStack.length});}
  checkRevision(revision){if(revision!==this.revision)throw Error('Project changed; reload before applying this edit');}
  edit(changes,revision){
    this.checkRevision(revision);
    if(!Array.isArray(changes)||!changes.length||changes.length>MAX_CHANGES)throw Error('Invalid project edits');
    const merged=new Map(this.document.changes.map(c=>[key(c),c]));
    for(const c of changes){checkChange(c);if(c.value===null)merged.delete(key(c));else merged.set(key(c),{kind:c.kind,id:c.id,field:c.field,value:c.value});}
    if(merged.size>MAX_CHANGES)throw Error('Too many project changes');
    const next=[...merged.values()].sort((a,b)=>key(a).localeCompare(key(b),'en'));
    if(JSON.stringify(next)===content(this.document))return this.snapshot();
    this.undoStack.push(this.document.changes);if(this.undoStack.length>MAX_HISTORY)this.undoStack.shift();
    this.document.changes=next;this.redoStack=[];this.revision++;return this.snapshot();
  }
  history(direction,revision){
    this.checkRevision(revision);
    if(!['undo','redo'].includes(direction))throw Error('Invalid history action');
    const from=direction==='undo'?this.undoStack:this.redoStack,to=direction==='undo'?this.redoStack:this.undoStack;
    if(from.length){to.push(this.document.changes);this.document.changes=from.pop();this.revision++;}return this.snapshot();
  }
  async save(file){
    const json=JSON.stringify(this.document,null,2),saved=content(this.document),temp=file+'.'+randomUUID()+'.tmp';
    await fs.mkdir(path.dirname(file),{recursive:true});
    try{await fs.writeFile(temp,json,{flag:'wx'});await fs.rename(temp,file);this.saved=saved;}
    finally{await fs.rm(temp,{force:true});}
    return this.snapshot();
  }
}
module.exports={Project};
