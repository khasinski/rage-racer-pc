const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');const {LauncherService,run}=require('../main/service.cjs');
function pack(){
 const blocks=Array.from({length:11},()=>Buffer.alloc(4));blocks[4]=Buffer.alloc(28);blocks[4].writeUInt32LE(1);blocks[4].writeUInt16LE(100,26);blocks[5].writeUInt32LE(1);
 blocks[2]=Buffer.alloc(8+4*48+3,0xa7);blocks[2].writeUInt32LE(0);blocks[2].writeUInt32LE(1000,4);
 for(let i=0;i<3;i++){const p=8+i*48;blocks[2].writeInt32LE(i*100,p);blocks[2].writeUInt16LE(15,p+40);blocks[2].writeUInt16LE(i,p+44);}blocks[2].writeInt32LE(-1,8+3*48);
 const header=Buffer.alloc(44);let offset=44;for(const [i,b]of blocks.entries()){header.writeUInt32LE(offset,i*4);offset+=b.length;}return Buffer.concat([header,...blocks]);
}
test('environment colors and whole timeline compile atomically, preserve reserved bytes and share undo',async t=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-env-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));const data=path.join(root,'data');await fs.mkdir(path.join(data,'raw'),{recursive:true});const source=path.join(data,'raw/asset_088.bin'),original=pack();await fs.writeFile(source,original);const archive=path.join(root,'RAGE.BIN');await fs.writeFile(archive,'environment source');const s=new LauncherService({root:path.join(root,'editor'),bin:path.resolve(__dirname,'../resources/bin'),config:path.resolve(__dirname,'../../rage-port.ini'),modsRoot:path.join(root,'mods')});await s.init();s.requireReady=()=>{};s.state.disc={archive,path:archive,data,region:'PAL'};
 let p=await s.project(),scene=await s.trackScene(88);assert.equal(scene.environment.cues.length,3);assert.equal(scene.environment.length,1000);
 // Cue 1 crosses cue 2's OLD time; the complete new timeline remains ordered.
 p=await s.saveEnvironment(88,[{index:1,values:{time:250,color_0:0x123456,duration:25,mode:4}},{index:2,values:{time:300,color_8:0xabcdef}}],p.revision);scene=await s.trackScene(88);assert.equal(scene.environment.cues[1].time,250);assert.equal(scene.environment.cues[1].colors[0],0x123456);assert.equal(scene.environment.cues[2].colors[8],0xabcdef);
 for(const entries of [[{index:0,values:{time:1}}],[{index:1,values:{time:400}}],[{index:1,values:{duration:-1}}],[{index:1,values:{mode:5}}],[{index:1,values:{color_0:0x1000000}}]])await assert.rejects(s.saveEnvironment(88,entries,p.revision),/Invalid track object edit/);
 assert.equal((await s.project()).revision,p.revision);await s.applyProject(p.revision);const pointer=JSON.parse(await fs.readFile(path.join(s.modsRoot,'active.json'))),bytes=await fs.readFile(path.join(s.modsRoot,'builds',pointer.build,'raw/asset_088.bin'));assert.equal(bytes.length,original.length);
 const allowed=new Set(),at=original.readUInt32LE(8)+8+48;for(const n of [0,1,2,3,4,5,6,40,41,44,45])allowed.add(at+n);for(const n of [0,1,2,3,36,37,38])allowed.add(at+48+n);
 for(let i=0;i<bytes.length;i++)if(!allowed.has(i))assert.equal(bytes[i],original[i],`unrelated byte ${i}`);assert.deepEqual(await fs.readFile(source),original);
 p=await s.projectHistory('undo',p.revision);assert.equal((await s.trackScene(88)).environment.cues[1].time,100);p=await s.projectHistory('redo',p.revision);assert.equal((await s.trackScene(88)).environment.cues[1].time,250);
});
test('atmosphere form names all nine colors and separates timing from basic colors',async()=>{
 const {environmentEditor}=await import('../renderer/tracks.mjs');const html=environmentEditor({revision:3,environment:{skyRow:0,length:1000,cues:[{time:0,duration:10,mode:0,colors:Array(9).fill(0x808080)}]}},88);
 for(const name of ['Fog','Sky top','Sky middle','Horizon','Sky bottom','Ground near top','Ground near bottom','Ground far top','Ground far bottom'])assert.ok(html.includes(name));assert.match(html,/data-revision="3"/);assert.match(html,/data-env-field="time"[^>]*readonly/);assert.match(html,/not rendering frames/);assert.equal(environmentEditor({},88),'');
});
