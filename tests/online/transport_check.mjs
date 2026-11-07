import http from 'node:http';
import {createHash} from 'node:crypto';
import {spawn} from 'node:child_process';
import assert from 'node:assert/strict';

const sockets=new Set();let pong=false,probe=false;
function frame(op,payload,fin=true,wide=false){
  const p=Buffer.from(payload),h=Buffer.alloc(wide?10:p.length<126?2:4);
  h[0]=(fin?128:0)|op;
  if(wide){h[1]=127;h.writeBigUInt64BE(BigInt(p.length),2);}
  else if(p.length<126)h[1]=p.length;
  else{h[1]=126;h.writeUInt16BE(p.length,2);}
  return Buffer.concat([h,p]);
}
const server=http.createServer(async(req,res)=>{
  let data='';for await(const part of req)data+=part;
  const {name}=JSON.parse(data);res.setHeader('content-type','application/json');res.end(JSON.stringify({token:name}));
});
server.on('connection',s=>{sockets.add(s);s.on('close',()=>sockets.delete(s));});
server.on('upgrade',(req,socket)=>{
  const key=req.headers['sec-websocket-key'];
  const accept=createHash('sha1').update(key+'258EAFA5-E914-47DA-95CA-C5AB0DC85B11').digest('base64');
  const mode=new URL(req.url,'http://local').searchParams.get('token');
  socket.write('HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: '+accept+'\r\n\r\n');
  if(mode==='bad'){socket.write(Buffer.from([0xc2,0]));return;}
  socket.write(frame(1,'{"t":"wel',false));
  socket.write(frame(9,'hi'));socket.write(frame(0,'come"}'));
  const payload=Buffer.alloc(4096);for(let i=0;i<payload.length;i++)payload[i]=i&255;payload[0]=2;payload[1]=77;
  socket.write(frame(2,payload,true,true));
  let input=Buffer.alloc(0),closing=false;
  socket.on('data',chunk=>{
    input=Buffer.concat([input,chunk]);
    while(input.length>=6){
      assert(input[1]&128,'clients must mask every frame');
      const length=input[1]&127;assert(length<126);
      if(input.length<6+length)return;
      const op=input[0]&15,mask=input.subarray(2,6),body=Buffer.from(input.subarray(6,6+length));
      for(let i=0;i<length;i++)body[i]^=mask[i&3];
      input=input.subarray(6+length);
      if(op===10){assert.equal(body.toString(),'hi');pong=true;}
      if(op===1){assert.equal(body.toString(),'{"t":"probe"}');probe=true;}
      if(pong&&probe&&!closing){closing=true;setTimeout(()=>socket.end(frame(8,Buffer.from([0x0f,0xa1]))),100);}
    }
  });
});
await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
try{
  for(const mode of ['good','bad']){
    const status=await new Promise((resolve,reject)=>{
      const child=spawn(process.argv[2],[`http://127.0.0.1:${server.address().port}`,mode],{stdio:'inherit'});
      child.on('error',reject);child.on('exit',resolve);
    });assert.equal(status,0,mode);
  }
  assert(pong&&probe);console.log('fragmentation, ping/pong, masking, 64-bit length, replacement and invalid RSV passed');
}finally{for(const socket of sockets)socket.destroy();server.close();}
