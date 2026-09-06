const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),os=require('node:os'),path=require('node:path');
const {inventory}=require('../main/mods.cjs');
test('native inventory preserves hidden-file policy, ordering and rejects oversized input',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-inventory-'));
 try{
  await assert.rejects(inventory(root),/empty/);
  await fs.mkdir(path.join(root,'.ignored'));await fs.writeFile(path.join(root,'.ignored','run.js'),'ignored');
  await fs.mkdir(path.join(root,'textures'));await fs.writeFile(path.join(root,'textures','z.png'),'z');
  await fs.writeFile(path.join(root,'mod.toml'),'[mod]');
  assert.deepEqual(await inventory(root),['mod.toml','textures/z.png']);
  const large=path.join(root,'textures','large.png');await fs.writeFile(large,'');
  await fs.truncate(large,128*1024*1024+1);
  await assert.rejects(inventory(root),/Unsupported mod file/);
  assert.equal((await fs.stat(large)).size,128*1024*1024+1);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
