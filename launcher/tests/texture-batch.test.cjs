const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os'),{inflateSync}=require('node:zlib');
const {run}=require('../main/service.cjs');
function pixels(png){const chunks=[];for(let at=8;at<png.length;){const n=png.readUInt32BE(at);if(png.toString('ascii',at+4,at+8)==='IDAT')chunks.push(png.subarray(at+8,at+8+n));at+=n+12;}return inflateSync(Buffer.concat(chunks));}
test('texture batch reuses uploads and samples windows without corrupting adjacent pixels',async t=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'rr-texture-batch-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));
 const raw=path.join(root,'pixels.bin'),data=Buffer.alloc(4);data.writeUInt16LE(31,0);data.writeUInt16LE(31<<10,2);await fs.writeFile(raw,data);
 const cli=path.resolve(__dirname,'../resources/bin/rage-mod-cli'+(process.platform==='win32'?'.exe':''));
 const input={uploads:[[raw,0,0,0,2,1]],textures:[[256,0,0,0,0,0],[256,0,1,1,1,0]]};
 await run(cli,['--texture-batch-stdin',root],{input:Buffer.from(JSON.stringify(input))});
 const full=pixels(await fs.readFile(path.join(root,'texture-0.png'))),window=pixels(await fs.readFile(path.join(root,'texture-1.png')));
 assert.deepEqual([...full.subarray(1,9)],[255,0,0,255,0,0,255,255]);
 assert.deepEqual([...window.subarray(1,9)],[0,0,255,255,0,0,255,255]);
 assert.deepEqual([...window.subarray(1026,1030)],[0,0,255,255]);
 const before=await fs.readFile(path.join(root,'texture-0.png'));
 await assert.rejects(run(cli,['--texture-batch-stdin',root],{input:Buffer.from(JSON.stringify(input))}),/Invalid texture batch/);
 assert.deepEqual(await fs.readFile(path.join(root,'texture-0.png')),before);
 const other=path.join(root,'invalid');await fs.mkdir(other);input.textures[1]=[256,0,256,1,1,0];
 await assert.rejects(run(cli,['--texture-batch-stdin',other],{input:Buffer.from(JSON.stringify(input))}),/Invalid texture batch/);assert.deepEqual(await fs.readdir(other),[]);
});
