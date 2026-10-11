const {test}=require('node:test'),assert=require('node:assert/strict');
const path=require('node:path'),fs=require('node:fs/promises'),os=require('node:os');
const {run}=require('../main/service.cjs');
const root=path.resolve(__dirname,'../..');
const cli=path.join(root,process.env.RAGE_LAUNCHER_BUILD_DIR||'build/release','rage-mod-cli'+(process.platform==='win32'?'.exe':''));
test('editor uses the game parser for both regional catalogs and rejects malformed values',async()=>{
  for(const name of ['cars.toml','cars.ntscj.toml']){
    const {cars}=JSON.parse(await run(cli,['--car-profile',path.join(root,name)]));
    assert.equal(cars.length,32);assert.equal(new Set(cars.map(c=>c.variant)).size,32);
    assert.equal(cars.find(c=>c.model===12&&c.grade===0).variant,31);
    assert.equal(typeof cars[0].price,'number');assert.equal(typeof cars[0].manual_only,'boolean');
  }
  const dir=await fs.mkdtemp(path.join(os.tmpdir(),'rage-car-profile-'));
  try{
    const file=path.join(dir,'cars.toml');
    const output=path.join(dir,'compiled.toml');
    await fs.writeFile(file,'[[cars]]\nid = "car_00_g0"\nmodel = 0\ngrade = 0\nprice = 0\nmanual_only = true\n');
    await run(cli,['--compose-car-profile',path.join(root,'cars.toml'),file,output]);
    const original=JSON.parse(await run(cli,['--car-profile',path.join(root,'cars.toml')])).cars;
    const compiled=JSON.parse(await run(cli,['--car-profile',output])).cars;
    assert.equal(compiled.length,32);assert.equal(compiled[0].price,0);assert.equal(compiled[0].manual_only,true);
    assert.deepEqual(compiled.slice(1),original.slice(1));
    await assert.rejects(run(cli,['--compose-car-profile',path.join(root,'cars.toml'),file,output]),/create/);
    assert.equal(JSON.parse(await run(cli,['--car-profile',output])).cars[0].price,0);
    await fs.writeFile(file,'[[cars]]\nid = "car_0"\nmodel = 0\ngrade = 4\nprice = 0\n');
    await assert.rejects(run(cli,['--car-profile',file]),/grade/);
    await fs.writeFile(file,'[[cars]]\nid = "car_0"\nmodel = 0\ngrade = 0\nprice = -1\n');
    await assert.rejects(run(cli,['--car-profile',file]),/metadata/);
    await assert.rejects(run(cli,['--compose-car-profile',path.join(root,'cars.toml'),file,path.join(dir,'invalid.toml')]),/metadata/);
    await assert.rejects(fs.access(path.join(dir,'invalid.toml')));
    await fs.writeFile(file,'[[cars]]\nid = "car_0"\nmodel = 0\ngrade = 0\nprice = 999999999999999999999999999999999999999\n');
    await assert.rejects(run(cli,['--car-profile',file]),/invalid value/);
  }finally{await fs.rm(dir,{recursive:true,force:true});}
});
