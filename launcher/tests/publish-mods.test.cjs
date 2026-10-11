const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {LauncherService,run}=require('../main/service.cjs');
test('saved selection is discoverable by the game and failed replacement preserves the previous mod',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-publish-'));
 const rename=fs.rename;
 try{
  const modsRoot=path.join(root,'game-mods');
  const service=new LauncherService({root:path.join(root,'editor'),modsRoot,bin:path.resolve(__dirname,'../resources/bin'),config:path.resolve(__dirname,'../resources/rage-port.ini')});
  const archive=path.join(root,'RAGE.BIN');await fs.writeFile(archive,'source revision');
  await service.init();service.state.disc={region:'NTSC-U',archive,path:archive,data:root};
  const source=path.join(root,'source');await fs.mkdir(path.join(source,'meshes'),{recursive:true});
  const obj=path.join(root,'body.obj');await fs.writeFile(obj,'v 0 0 0\nv 1 0 0\nv 0 1 0\nvt 0 0\nvn 0 0 1\nusemtl rage_0\nf 1/1/1 2/1/1 3/1/1\n');
  await run(service.tool('rage-mesh-obj'),['import',obj,path.join(source,'meshes/body.rmesh')]);
  const original='[meshes]\n"car.player.10.part.0" = "meshes/body.rmesh"\n';await fs.writeFile(path.join(source,'mod.toml'),original);
  await service.importMod(source);const mod=service.state.mods[0];await service.toggleMod(mod.id,true);
  const target=await service.publishMods();
  const pointer=path.join(modsRoot,'active.json'),firstPointer=await fs.readFile(pointer,'utf8');
  assert.equal(path.dirname(target),path.join(modsRoot,'builds'));
  const before=await fs.readFile(path.join(target,'mod.toml'));
  const decoded=JSON.parse(await run(service.tool('rage-mod-cli'),[path.join(target,'mod.toml')]));
  assert.ok(decoded.meshes['car.player.10.part.0']);
  await run(service.tool('rage-mod-cli'),['--mesh',path.join(target,decoded.meshes['car.player.10.part.0'])]);
  assert.equal(await fs.readFile(path.join(source,'mod.toml'),'utf8'),original);
  await service.saveMaterial(mod.id,'car.0.material.0','lit opaque 0.5 0 1 1 1 1 0 0 0',(await service.project()).revision);
  fs.rename=async(from,to)=>{if(to===pointer)throw Object.assign(Error('Disk full'),{code:'ENOSPC'});return rename(from,to);};
  await assert.rejects(service.publishMods(),/Disk full/);
  assert.deepEqual(await fs.readFile(path.join(target,'mod.toml')),before);
  assert.equal(await fs.readFile(pointer,'utf8'),firstPointer);
  fs.rename=rename;const next=await service.publishMods();
  assert.notEqual(next,target);
  assert.ok((await fs.readFile(path.join(next,'mod.toml'),'utf8')).includes('0.5'));
  assert.deepEqual(await fs.readFile(path.join(target,'mod.toml')),before);
  assert.equal((await fs.readdir(path.dirname(target))).some(n=>n.startsWith('.')),false);
  assert.equal((await fs.readdir(service.root)).some(n=>n.startsWith('active-mods-')||n.startsWith('mod-sources-')),false);
  await fs.unlink(path.join(modsRoot,'.rage-editor.json'));
  await assert.rejects(service.publishMods(),/unmanaged content/);
  assert.ok(await fs.readFile(path.join(target,'mod.toml')));
  service.game={};await assert.rejects(service.publishMods(),/Close the game/);
 }finally{
  fs.rename=rename;
  await fs.rm(root,{recursive:true,force:true});
 }
});
