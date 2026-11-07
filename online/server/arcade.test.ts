import { test } from 'node:test';
import assert from 'node:assert/strict';
import { ArcadeSession } from './arcade.ts';
import { Simulation } from './sim.ts';
import type { Client } from './room.ts';
import type { ServerMessage } from '../shared/protocol.ts';

class FakeSimulation {
  tickNumber=0; created=0; freed=0; phase=2; finished=new Set<number>();
  auto=Array(8).fill(false) as boolean[]; applied: number[]=[];
  speeds=Array(12).fill(400) as number[];
  classCar(_class: number,model: number):number{return model===3?6:-1;}
  carAllowed(_class: number,variant: number):boolean{return variant===6;}
  carAutomatic():boolean{return true;}
  courseAllowed(cls: number,course: number):boolean{return course>=0&&course<4&&(course!==3||cls>=2);}
  maxHumans():number{return 8;}
  createRace():number{this.created++;this.tickNumber=0;this.phase=2;this.finished.clear();return this.created;}
  start():boolean{return true;}
  free():void{this.freed++;}
  autopilot(_race: number,seat: number,on: boolean):boolean{this.auto[seat]=on;return true;}
  simTick():number{return this.tickNumber;}
  tick():number{this.tickNumber++;return this.phase;}
  frame():Uint8Array{return new Uint8Array([1]);}
  seat(_race: number,seat: number){return {status:seat>=12?0:this.finished.has(seat)?2:1,place:seat+1,timeMs:this.tickNumber*20,bestLapMs:-1,lap:1,speed:this.speeds[seat]};}
  setInput(_race: number,seat: number):boolean{if(this.auto[seat])return false;this.applied.push(seat);return true;}
}
type Player=Client & {messages:(ServerMessage|Buffer)[]};
function player(id:number):Player{
  const messages:(ServerMessage|Buffer)[]=[];
  return {user:{id,name:`Racer${id}`,guest:true,admin:false},roomId:null,latencyMs:0,messages,
    ws:{OPEN:1,readyState:1,bufferedAmount:0,send:(data:string|Buffer)=>messages.push(typeof data==='string'?JSON.parse(data) as ServerMessage:data)} } as unknown as Player;
}
function messages<T extends ServerMessage['t']>(p:Player,t:T):Extract<ServerMessage,{t:T}>[]{
  return p.messages.filter((m):m is Extract<ServerMessage,{t:T}>=>!Buffer.isBuffer(m)&&m.t===t);
}
function input(sequence=1):Buffer{const b=Buffer.alloc(41);b[0]=1;b.writeUInt32LE(sequence,1);b.writeInt32LE(1,9);b.writeInt32LE(256,25);return b;}
function fixture(){
  const sim=new FakeSimulation();let now=1_000_000;
  const arcade=new ArcadeSession(sim as unknown as Simulation,()=>now);
  const advance=(n:number)=>{for(let i=0;i<n;i++){now+=20;arcade.tick(now);}};
  return {sim,arcade,advance,now:()=>now,jump:(ms:number)=>{now+=ms;arcade.tick(now);}};
}
test('solo starts with CPU, loaded acknowledgement gates handover at one tick',()=>{
  const f=fixture(),a=player(1);f.arcade.join(a);
  const start=messages(a,'raceStart')[0];assert(start.arcade);assert.equal(start.humans,8);assert.equal(start.localSeat,-1);assert.equal(f.sim.created,1);
  f.arcade.loaded(a,start.epoch!+1,true);assert.equal(messages(a,'arcadeControl').length,0);
  f.arcade.input(a,input());assert.deepEqual(f.sim.applied,[]);
  f.arcade.loaded(a,start.epoch!,true);assert.equal(messages(a,'arcadeControl')[0].activateAt,150);
  f.advance(149);assert(f.sim.auto[0]);f.advance(1);assert(!f.sim.auto[0]);
  const packet=a.messages.find(Buffer.isBuffer)!;assert.equal(packet[0],5);assert.equal(packet.readUInt32LE(1),1);
  f.arcade.input(a,input());f.advance(1);assert.deepEqual(f.sim.applied,[0]);
  f.arcade.leave(a);
});
test('late join uses same race; departure restores CPU; reconnect reserves seat',()=>{
  const f=fixture(),a=player(1),b=player(2);f.arcade.join(a);f.arcade.loaded(a,1,true);f.advance(200);
  f.arcade.join(b);f.arcade.loaded(b,1,true);
  const control=messages(b,'arcadeControl')[0];assert.equal(control.epoch,1);assert(control.late);assert.equal(f.sim.created,1);assert.notEqual(control.seat,0);
  f.advance(150);assert(!f.sim.auto[control.seat]);f.arcade.disconnect(b,f.now());assert(f.sim.auto[control.seat]);
  const c=player(3);f.arcade.join(c);f.arcade.loaded(c,1,true);assert.notEqual(messages(c,'arcadeControl')[0].seat,control.seat);
  const back=player(2);f.arcade.join(back);f.arcade.loaded(back,1,true);assert.equal(messages(back,'arcadeControl')[0].seat,control.seat);
  f.arcade.input(b,input(100));f.advance(150);assert(!f.sim.applied.includes(control.seat));
  f.arcade.input(back,input());f.advance(1);assert(f.sim.applied.includes(control.seat));
  for(const p of [a,back,c])f.arcade.leave(p);
});
test('results, persistent chat, voting and next start need no host or ready flags',()=>{
  const f=fixture(),a=player(1);f.arcade.join(a);f.arcade.loaded(a,1,true);f.advance(150);
  f.arcade.message(a,'hello community');f.arcade.vote(a,2);f.sim.phase=3;f.advance(1);
  assert.equal(messages(a,'results').length,1);assert.equal(f.sim.freed,1);
  const b=player(2);f.arcade.join(b);assert(messages(b,'arcadeChat')[0].lines.some(l=>l.text==='hello community'));
  f.jump(15000);assert.equal(f.sim.created,2);
  const start=messages(a,'raceStart').at(-1)!;assert.equal(start.epoch,2);assert.equal(start.settings.course,2);
  f.arcade.loaded(a,1,true);assert.equal(messages(a,'arcadeControl').length,1);
  f.arcade.loaded(a,2,true);assert.equal(messages(a,'arcadeControl').at(-1)!.epoch,2);
  f.arcade.leave(a);f.arcade.leave(b);
});
test('late arrivals near finish wait with the community; empty server sleeps',()=>{
  const f=fixture(),a=player(1);f.arcade.join(a);f.arcade.loaded(a,1,true);f.advance(200);
  f.sim.finished.add(3);f.advance(1);f.jump(11000);
  const b=player(2);f.arcade.join(b);f.arcade.loaded(b,1,true);assert.equal(messages(b,'arcadeControl').length,0);
  f.arcade.disconnect(a,f.now());f.arcade.disconnect(b,f.now());f.jump(15001);
  assert.equal(f.sim.freed,1);assert.equal(f.arcade.tick(f.now()),false);
});
test('failed load releases its reserved place; bounded chat is retained',()=>{
  const f=fixture(),a=player(1),b=player(2);f.arcade.join(a);f.arcade.loaded(a,1,true);f.advance(151);f.arcade.join(b);f.arcade.loaded(b,1,false);
  assert(!f.arcade.has(2));
  for(let i=0;i<80;i++){f.jump(501);f.arcade.message(a,`line ${i}`);}
  const c=player(3);f.arcade.join(c);assert.equal(messages(c,'arcadeChat')[0].lines.length,64);
  f.arcade.leave(a);f.arcade.leave(c);
});
test('new drivers receive a moving CPU car before a closer stalled one',()=>{
  const f=fixture(),a=player(1),b=player(2);f.arcade.join(a);f.arcade.loaded(a,1,true);f.advance(150);
  f.sim.speeds.fill(0);f.sim.speeds[4]=400;
  f.arcade.join(b);f.arcade.loaded(b,1,true);assert.equal(messages(b,'arcadeControl')[0].seat,4);
  f.arcade.leave(a);f.arcade.leave(b);
});
test('real C simulation preserves the complete car/checkpoint across CPU takeover',{skip:!process.env.RAGE_TEST_DISC},async()=>{
  const sim=await Simulation.load(process.env.RAGE_TEST_DISC!);
  const variant=sim.classCar(0,3),race=sim.createRace(0,0,false,2,true,Array.from({length:8},()=>({variant,manual:false})));
  assert(race);assert(sim.autopilot(race,0,true));assert(sim.start(race));
  for(let tick=0;tick<450;tick++)sim.tick(race);
  const before=sim.frame(race)!;assert(before);
  assert(sim.autopilot(race,0,false));const after=sim.frame(race)!;
  // Checkpoint v1: 29-byte race header; first driver's 88-byte event/state
  // prefix, 28-byte input, then all 412 car bytes. Only ownership input resets.
  const a=before.slice(),b=after.slice();a.fill(0,29+88,29+116);b.fill(0,29+88,29+116);assert.deepEqual(a,b);
  assert(!sim.setInput(race,0,new Int32Array([3,0,0,0,256,0,0,0])));
  assert(!sim.setInput(race,0,new Int32Array([4,0,0,0,256,0,0,0])));
  assert(sim.setInput(race,0,new Int32Array([1,0,0,0,256,0,0,0])));
  sim.tick(race);sim.tick(race);assert(sim.frame(race));
  assert(sim.autopilot(race,0,true));assert(!sim.setInput(race,0,new Int32Array([1,0,0,0,256,0,0,0])));
  for(let tick=0;tick<50;tick++)sim.tick(race);assert(sim.frame(race));sim.free(race);
});
