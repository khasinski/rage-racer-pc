const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {LauncherService,run}=require('../main/service.cjs');
test('saved selection is discoverable by the game and failed replacement preserves the previous mod',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rage-publish-'));
 const old=Object.fromEntries(['HOME','APPDATA','XDG_CONFIG_HOME'].map(k=>[k,process.env[k]]));
 for(const k of Object.keys(old))process.env[k]=root;
 const rename=fs.rename;
 try{
  const service=new LauncherService({root:path.join(root,'editor'),bin:path.resolve(__dirname,'../resources/bin'),config:path.resolve(__dirname,'../resources/rage-port.ini')});
  await service.init();service.state.disc={region:'NTSC-U'};
  const source=path.join(root,'source');await fs.mkdir(path.join(source,'meshes'),{recursive:true});
  const obj=path.join(root,'body.obj');await fs.writeFile(obj,'v 0 0 0\nv 1 0 0\nv 0 1 0\nvt 0 0\nvn 0 0 1\nusemtl rage_0\nf 1/1/1 2/1/1 3/1/1\n');
  await run(service.tool('rage-mesh-obj'),['import',obj,path.join(source,'meshes/body.rmesh')]);
  const original='[meshes]\n"car.player.10.part.0" = "meshes/body.rmesh"\n';await fs.writeFile(path.join(source,'mod.toml'),original);
  await service.importMod(source);const mod=service.state.mods[0];await service.toggleMod(mod.id,true);
  const target=await service.publishMods();
  const expected=JSON.parse(await run(service.tool('rage-mod-cli'),['--mod-directory'])).directory;assert.equal(target,expected);
  const before=await fs.readFile(path.join(target,'mod.toml'));
  const decoded=JSON.parse(await run(service.tool('rage-mod-cli'),[path.join(target,'mod.toml')]));
  assert.ok(decoded.meshes['car.player.10.part.0']);
  await run(service.tool('rage-mod-cli'),['--mesh',path.join(target,decoded.meshes['car.player.10.part.0'])]);
  assert.equal(await fs.readFile(path.join(source,'mod.toml'),'utf8'),original);
  await service.saveMaterial(mod.id,'car.0.material.0','lit opaque 0.5 0 1 1 1 1 0 0 0');
  fs.rename=async(from,to)=>{if(from.startsWith(target+'.staging-')&&to===target)throw Object.assign(Error('Disk full'),{code:'ENOSPC'});return rename(from,to);};
  await assert.rejects(service.publishMods(),/Disk full/);
  assert.deepEqual(await fs.readFile(path.join(target,'mod.toml')),before);
  fs.rename=rename;await service.publishMods();
  assert.ok((await fs.readFile(path.join(target,'mod.toml'),'utf8')).includes('0.5'));
  assert.equal((await fs.readdir(path.dirname(target))).some(n=>n.includes('.staging-')||n.includes('.previous-')),false);
  assert.equal((await fs.readdir(service.root)).some(n=>n.startsWith('active-mods-')||n.startsWith('mod-sources-')),false);
  await fs.unlink(path.join(target,'.rage-editor.json'));
  await assert.rejects(service.publishMods(),/manually installed mod/);
  assert.ok(await fs.readFile(path.join(target,'mod.toml')));
  service.game={};await assert.rejects(service.publishMods(),/Close the game/);
 }finally{
  fs.rename=rename;for(const[k,v]of Object.entries(old)){if(v===undefined)delete process.env[k];else process.env[k]=v;}
  await fs.rm(root,{recursive:true,force:true});
 }
});
