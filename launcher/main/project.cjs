const fs=require('node:fs/promises');
const {randomUUID}=require('node:crypto');
const path=require('node:path');

const MAX_CHANGES=10000,MAX_HISTORY=200,MAX_BYTES=8*1024*1024,MAX_HISTORY_BYTES=32*1024*1024;
const kinds=new Set(['car','track','interface','audio']);
const safeName=s=>typeof s==='string'&&/^[a-z][a-z0-9_./-]{0,159}$/.test(s)&&!s.split(/[./]/).some(x=>['__proto__','prototype','constructor'].includes(x));
function checkSource(source){
  if(!source||!['PAL','NTSC-U','NTSC-J'].includes(source.region)||!/^sha256:[a-f0-9]{64}$/.test(source.fingerprint))throw Error('Invalid project source');
}
function checkChange(c){
  if(!c||!kinds.has(c.kind)||!safeName(c.id)||!safeName(c.field)||
     !(c.value===null||typeof c.value==='boolean'||typeof c.value==='string'&&c.value.length<=4096||typeof c.value==='number'&&Number.isFinite(c.value)||Array.isArray(c.value)&&c.value.length>0&&c.value.length<=16&&c.value.every(Number.isSafeInteger)))throw Error('Invalid project change');
}
const key=c=>`${c.kind}:${c.id}:${c.field}`;
const draft=d=>({changes:d.changes,selection:d.selection});
const content=d=>JSON.stringify(draft(d));
const frame=d=>({value:draft(d),bytes:Buffer.byteLength(content(d))});
const packageId=id=>typeof id==='string'&&/^[a-z0-9][a-z0-9_-]{0,79}$/.test(id)&&!['constructor','prototype'].includes(id);
function selection(input){
  if(!input||typeof input!=='object'||Array.isArray(input)||!Array.isArray(input.packages)||input.packages.length>128||
     input.packages.some(id=>!packageId(id))||new Set(input.packages).size!==input.packages.length||
     !input.resolutions||typeof input.resolutions!=='object'||Array.isArray(input.resolutions)||
     ![Object.prototype,null].includes(Object.getPrototypeOf(input.resolutions)))throw Error('Invalid project package selection');
  const choices=Object.entries(input.resolutions).sort(([a],[b])=>a.localeCompare(b,'en'));
  if(choices.length>10000)throw Error('Too many project priorities');
  const resolutions=Object.fromEntries(choices.map(([key,choice])=>{
    if(key.length>512||!/^(?:raw\/(?:asset_[0-9]{3}\.bin|course_(?:0[1-9]|[12][0-9]|3[0-2])\.txt)|textures\/[A-Za-z0-9_./-]+|(?:texture|material|mesh|audio|legacy-textures):[a-z0-9_.-]+)$/.test(key)||key.split('/').some(part=>!part||part==='.'||part==='..')||
       !choice||!Array.isArray(choice.candidates)||choice.candidates.length<2||choice.candidates.length>128||
       choice.candidates.some(id=>!packageId(id))||new Set(choice.candidates).size!==choice.candidates.length||
       !choice.candidates.includes(choice.winner))throw Error('Invalid project priority');
    return [key,{winner:choice.winner,candidates:[...choice.candidates].sort()}];
  }));
  return {packages:[...input.packages].sort(),resolutions};
}

// One history for edits across every screen. Binary parsing and gameplay
// validation belong to the compiled codecs, not to this document container.
class Project {
  constructor(source,name='My project',selected={packages:[],resolutions:{}}){
    checkSource(source);
    if(typeof name!=='string'||!name.trim()||name.length>120)throw Error('Invalid project name');
    this.document={format:2,id:randomUUID(),name,source:{region:source.region,fingerprint:source.fingerprint},changes:[],selection:selection(selected)};
    this.checkSize(this.document);
    this.revision=0;this.undoStack=[];this.redoStack=[];this.saved=content(this.document);
  }
  static selection(input){return selection(input);}
  static async open(file,source,legacySelection={packages:[],resolutions:{}}){
    checkSource(source);
    const handle=await fs.open(file,'r');let d;
    try{if((await handle.stat()).size>MAX_BYTES)throw Error('Project exceeds size limit');d=JSON.parse(await handle.readFile('utf8'));}finally{await handle.close();}
    if(![1,2].includes(d.format)||typeof d.id!=='string'||!/^[a-f0-9]{8}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{12}$/.test(d.id)||typeof d.name!=='string'||d.name.length>120||!Array.isArray(d.changes)||d.changes.length>MAX_CHANGES)throw Error('Invalid project document');
    checkSource(d.source);
    if(d.source.region!==source.region||d.source.fingerprint!==source.fingerprint)throw Error('Project belongs to a different disc revision');
    const seen=new Set();for(const c of d.changes){checkChange(c);if(c.value===null)throw Error('Reset entries must not be stored');if(seen.has(key(c)))throw Error('Duplicate project change');seen.add(key(c));}
    const p=new Project(source,d.name,selection(d.format===1?legacySelection:d.selection));
    p.document.id=d.id;p.document.changes=d.changes.map(c=>({kind:c.kind,id:c.id,field:c.field,value:c.value}));p.checkSize(p.document);p.saved=content(p.document);return p;
  }
  snapshot(){return structuredClone({...this.document,revision:this.revision,dirty:content(this.document)!==this.saved,canUndo:!!this.undoStack.length,canRedo:!!this.redoStack.length});}
  checkRevision(revision){if(revision!==this.revision)throw Error('Project changed; reload before applying this edit');}
  edit(changes,revision){
    this.checkRevision(revision);
    if(!Array.isArray(changes)||!changes.length||changes.length>MAX_CHANGES)throw Error('Invalid project edits');
    const merged=new Map(this.document.changes.map(c=>[key(c),c]));
    for(const c of changes){checkChange(c);if(c.value===null)merged.delete(key(c));else merged.set(key(c),{kind:c.kind,id:c.id,field:c.field,value:structuredClone(c.value)});}
    if(merged.size>MAX_CHANGES)throw Error('Too many project changes');
    const next=[...merged.values()].sort((a,b)=>key(a).localeCompare(key(b),'en'));
    return this.commit({changes:next,selection:this.document.selection});
  }
  select(input,revision){
    this.checkRevision(revision);
    return this.commit({changes:this.document.changes,selection:selection(input)});
  }
  commit(next){
    if(content(next)===content(this.document))return this.snapshot();
    this.checkSize({...this.document,...next});
    this.undoStack.push(frame(this.document));
    Object.assign(this.document,next);this.redoStack=[];this.trimHistory();this.revision++;return this.snapshot();
  }
  usesPackage(id){
    return [this.document,JSON.parse(this.saved),...[...this.undoStack,...this.redoStack].map(frame=>frame.value)].some(d=>d.selection.packages.includes(id));
  }
  trimHistory(){
    let bytes=[...this.undoStack,...this.redoStack].reduce((sum,frame)=>sum+frame.bytes,0);
    while(this.undoStack.length+this.redoStack.length>MAX_HISTORY||bytes>MAX_HISTORY_BYTES){
      const stack=this.undoStack.length?this.undoStack:this.redoStack;
      bytes-=stack.shift().bytes;
    }
  }
  checkSize(document){
    if(Buffer.byteLength(JSON.stringify(document,null,2))>MAX_BYTES)throw Error('Project exceeds size limit');
  }
  history(direction,revision){
    this.checkRevision(revision);
    if(!['undo','redo'].includes(direction))throw Error('Invalid history action');
    const from=direction==='undo'?this.undoStack:this.redoStack,to=direction==='undo'?this.redoStack:this.undoStack;
    if(from.length){to.push(frame(this.document));Object.assign(this.document,from.pop().value);this.trimHistory();this.revision++;}return this.snapshot();
  }
  async save(file){
    this.checkSize(this.document);
    const json=JSON.stringify(this.document,null,2),saved=content(this.document),temp=file+'.'+randomUUID()+'.tmp';
    await fs.mkdir(path.dirname(file),{recursive:true});
    try{await fs.writeFile(temp,json,{flag:'wx'});await fs.rename(temp,file);this.saved=saved;}
    finally{await fs.rm(temp,{force:true});}
    return this.snapshot();
  }
}
module.exports={Project};
