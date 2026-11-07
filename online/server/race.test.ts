import {test} from 'node:test';
import assert from 'node:assert/strict';
import {inflateRawSync} from 'node:zlib';
import {Race} from './race.ts';
import type {Simulation} from './sim.ts';
import type {Store} from './db.ts';
import type {Client} from './room.ts';

test('private races preserve legacy web frames while desktop receives epochs, on WS and RTC',()=>{
  for(const rtc of [false,true]){
    let tick=0;
    const sim={createRace:()=>1,seat:()=>({status:1,place:1,timeMs:0,bestLapMs:-1,lap:0}),start:()=>true,
      simTick:()=>tick,tick:()=>{tick++;return 2;},frame:()=>new Uint8Array([11,22,33]),free:()=>{}} as unknown as Simulation;
    const store={createRace:()=>42} as unknown as Store;
    const received=[[] as Buffer[],[] as Buffer[]];
    const clients=[false,true].map((native,i)=>({native,user:{id:i+1},
      ws:{OPEN:1,readyState:1,bufferedAmount:0,send:(p:Buffer)=>received[i].push(p)},
      rtc:rtc?{open:true,send:(p:Buffer)=>{received[i].push(inflateRawSync(p));return true;}}:undefined})) as unknown as Client[];
    const race=Race.create(sim,store,{broadcast:()=>{},client:id=>clients[id-1]},1,
      {name:'Compatibility',classIndex:0,course:0,reverse:false,laps:1,rivals:true,maxPlayers:2},
      clients.map((_c,i)=>({userId:i+1,name:`Racer${i}`,variant:6,manual:false,tire:0,paint:null,logo:null})))!;
    for(let id=1;id<=2;id++){race.watch(id);race.loaded(id);}
    race.step(1000);race.step(1020);
    const legacy=received[0][0],native=received[1][0];
    assert(legacy&&native);assert.equal(legacy[0],2);assert.equal(native[0],4);assert.equal(native.readUInt32LE(1),42);
    assert.deepEqual(legacy,Buffer.concat([Buffer.from([2]),native.subarray(5)]));
    race.free();
  }
});
