// Actual native client + real C/WASM checkpoint data, with deterministic
// finish timing. Results deliberately precede the final frame, as TCP and
// unreliable race-state delivery can arrive in either order.
import http from 'node:http';
import {spawn} from 'node:child_process';
import {createRequire} from 'node:module';
import assert from 'node:assert/strict';
import {Simulation} from '../../online/server/sim.ts';
const require=createRequire(new URL('../../online/package.json',import.meta.url));
const {WebSocketServer}=require('ws');
const disc=process.env.RAGE_TEST_DISC;
if(!disc)process.exit(77);
const sim=await Simulation.load(disc),variant=sim.classCar(0,3);
const settings={name:'Lifecycle check',classIndex:0,course:0,reverse:false,laps:1,rivals:false,maxPlayers:1};
const server=http.createServer((req,res)=>{
  res.writeHead(200,{'Content-Type':'application/json'});
  res.end(JSON.stringify({token:'fixture',user:{id:1,name:'Racer'}}));
});
const sockets=new WebSocketServer({server});
let race=0,epoch=0,interval,roundTimer,finalTimer,duplicateTimer;
let inputDuringResults=0,resultsSent=0;
function packet(frame,tick){
  const out=Buffer.alloc(22+frame.length);out[0]=5;out.writeUInt32LE(epoch,1);out.writeUInt32LE(tick,5);out[9]=1;
  out.set(frame,22);return out;
}
sockets.on('connection',ws=>{
  const send=message=>ws.send(JSON.stringify(message));
  let loaded=false;
  function round(){
    epoch++;loaded=false;resultsSent=0;
    race=sim.createRace(0,0,false,1,false,[{variant,manual:false}]);assert(race);
    assert(sim.autopilot(race,0,true));assert(sim.start(race));
    send({t:'raceStart',settings,seats:[{variant,manual:false,tire:0,name:'Racer'}],humans:1,localSeat:-1,arcade:true,epoch,viewSeat:0});
    send({t:'raceGo'});
    interval=setInterval(()=>{
      sim.tick(race);const tick=sim.simTick(race);
      if(loaded&&tick%2===0)ws.send(packet(sim.frame(race),tick));
      if(epoch!==1||tick<400)return;
      clearInterval(interval);
      // Retiring supplies an otherwise valid final checkpoint (including
      // activeFlag=-1); convert its single seat to a classified finisher.
      assert(sim.retire(race,0));sim.tick(race);
      const final=Buffer.from(sim.frame(race)),elapsed=final.readUInt32LE(21);
      final.writeInt32LE(1,9); // RaceFrame.finishCount
      final.writeInt32LE(1,29+60); // DriverFrame.place
      final.writeInt32LE(2,29+68); // DriverFrame.status = finished
      final.writeUInt32LE(elapsed,29+16); // DriverFrame.finishTick
      const finalPacket=packet(final,sim.simTick(race));sim.free(race);race=0;
      const results={t:'results',results:[{seat:0,userId:1,name:'Racer',variant,place:1,timeMs:elapsed*20,bestLapMs:-1,joinedLate:false,status:'finished'}]};
      resultsSent=Date.now();send(results);
      finalTimer=setTimeout(()=>ws.send(finalPacket),100);
      duplicateTimer=setTimeout(()=>send(results),200);
      send({t:'arcadeState',epoch,nextInMs:15000,nextCourse:0,nextClass:0,members:[{userId:1,name:'Racer',seat:0,online:true}]});
      roundTimer=setTimeout(round,15000);
    },20);
  }
  send({t:'welcome',discId:sim.discId,user:{id:1,name:'Racer'}});
  ws.on('message',(data,binary)=>{
    if(binary){if(resultsSent&&Date.now()-resultsSent>300)inputDuringResults++;return;}
    const message=JSON.parse(String(data));
    if(message.t==='joinArcade')round();
    if(message.t==='arcadeLoaded'&&message.epoch===epoch){
      assert(message.ok);loaded=true;
      send({t:'arcadeControl',epoch,seat:0,activateAt:150,late:false});
    }
  });
});
await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
const url=`http://127.0.0.1:${server.address().port}`;
const smoke=process.argv[3]==='smoke';
const args=smoke?['--set',`disc.image=${disc}`,'--set',`online.server=${url}`,'--set','hooks.online=arcade','--set','hooks.online_seconds=29','--set','run.frames=12000',...process.argv.slice(4)]
  :[disc,url,'arcade-cycle','35','flow'];
const child=spawn(process.argv[2],args,{env:{...process.env,PSYZ_HEADLESS:'1',SDL_AUDIODRIVER:'dummy'}});
let output='';child.stdout.on('data',bytes=>{output+=bytes;});child.stderr.on('data',bytes=>{output+=bytes;});
const timeout=setTimeout(()=>child.kill(),40000);
const code=await new Promise(resolve=>child.on('exit',resolve));
clearTimeout(timeout);clearInterval(interval);clearTimeout(roundTimer);clearTimeout(finalTimer);clearTimeout(duplicateTimer);
for(const client of sockets.clients)client.terminate();
await new Promise(resolve=>sockets.close(resolve));await new Promise(resolve=>server.close(resolve));
if(race)sim.free(race);
process.stdout.write(output);
assert.equal(code,0);assert(epoch>=2);assert.equal(inputDuringResults,0,'no driving input after results');
if(!smoke){assert.match(output,/RESULTS received/);assert.match(output,/ARCADE next round/);}
