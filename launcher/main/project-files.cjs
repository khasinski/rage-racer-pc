const fs=require('node:fs/promises');
const path=require('node:path');
const {createHash,randomUUID}=require('node:crypto');

const LIMIT=128*1024*1024;
const digest=data=>createHash('sha256').update(data).digest('hex');
const parse=ref=>{
  if(typeof ref!=='string'||!/^sha256:[a-f0-9]{64}\.(rmesh|png)$/.test(ref))throw Error('Invalid project model reference');
  return ref.slice(7);
};

// Only validated native models enter this store. References survive history
// changes; no editable library package owns the bytes used by a project.
class ProjectFiles {
  constructor(projectFile){this.root=path.join(path.dirname(projectFile),'files');}
  async directory(){
    await fs.mkdir(this.root,{recursive:true});
    const stat=await fs.lstat(this.root);
    if(!stat.isDirectory()||stat.isSymbolicLink())throw Error('Invalid project files directory');
  }
  async read(ref){
    const name=parse(ref);
    await this.directory();
    const file=path.join(this.root,name),stat=await fs.lstat(file);
    if(!stat.isFile()||stat.isSymbolicLink()||stat.size>LIMIT)throw Error('Invalid project model file');
    const data=await fs.readFile(file);
    if(data.length>LIMIT||digest(data)!==name.slice(0,64))throw Error('Project model has changed or is damaged');
    return data;
  }
  async put(file,type='rmesh'){
    if(!['rmesh','png'].includes(type))throw Error('Invalid project file type');
    const handle=await fs.open(file,'r');let data;
    try{
      const stat=await handle.stat();
      if(!stat.isFile()||stat.size>LIMIT)throw Error('Project model exceeds size limit');
      data=await handle.readFile();
    }finally{await handle.close();}
    if(data.length>LIMIT)throw Error('Project model exceeds size limit');
    const ref=`sha256:${digest(data)}.${type}`,name=parse(ref);
    await this.directory();
    const temp=path.join(this.root,randomUUID()+'.tmp');
    try{
      await fs.writeFile(temp,data,{flag:'wx',mode:0o444});
      try{await fs.link(temp,path.join(this.root,name));}
      catch(error){if(error.code!=='EEXIST')throw error;}
      await this.read(ref); // Never accept a corrupt pre-existing object.
      return ref;
    }finally{await fs.rm(temp,{force:true});}
  }
}
module.exports={ProjectFiles};
