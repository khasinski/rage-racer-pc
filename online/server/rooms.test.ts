import {test} from 'node:test';
import assert from 'node:assert/strict';
import {Lobby} from './rooms.ts';
import {checkDuelCode} from './room-rules.ts';
import type {Client, Room} from './room.ts';
import type {Race} from './race.ts';
import type {Simulation} from './sim.ts';
import type {Store} from './db.ts';
import type {ServerMessage} from '../shared/protocol.ts';

test('duel codes accept only 12 base64url characters and preserve case',()=>{
  for(const bad of [null,undefined,12,{},'', 'short','AbCd_12-4567x','AbCd 123-4567','AbCd/123-4567','AbCd_123-45ą'])
    assert(checkDuelCode(bad));
  assert.equal(checkDuelCode('AbCd_12-4567'),null);
  assert.equal(checkDuelCode(' \nAbCd_12-4567\t'),null);
});

test('duel protocol validates format, availability and membership before moving a player',()=>{
  let nextId=0;
  const sim={courseAllowed:()=>true,maxHumans:()=>2,carModels:()=>1,classCar:()=>0,carAutomatic:()=>true} as unknown as Simulation;
  const store={createRoom:()=>++nextId} as unknown as Store;
  const lobby=new Lobby(sim,store);
  function player(id:number){
    const messages:ServerMessage[]=[];
    const client={user:{id,name:`Racer ${id}`},roomId:null,latencyMs:null,
      ws:{OPEN:1,readyState:1,send:(text:string)=>messages.push(JSON.parse(text))}} as unknown as Client;
    const request=(t:string,extra:Record<string,unknown>={})=>lobby.message(client,Buffer.from(JSON.stringify({t,...extra})),false);
    const error=()=>{const m=messages.at(-1);assert.equal(m?.t,'error');return (m as {message:string}).message;};
    return {client,messages,request,error};
  }
  const host=player(1),guest=player(2),third=player(3);
  host.request('createDuel');
  const state=host.messages.findLast(m=>m.t==='room');
  assert(state?.t==='room'&&state.room?.duel);
  const code=state.room.duel.token;
  assert.equal(checkDuelCode(code),null);
  third.request('createDuel');const priorRoom=third.client.roomId;
  for(const token of [null,'short','AbCd/123-4567']){
    third.request('joinDuel',{token});assert(third.error());assert.equal(third.client.roomId,priorRoom);
  }
  third.request('joinDuel',{token:'____________'});assert.match(third.error(),/invalid or no longer active/);
  guest.request('joinDuel',{token:` \n${code}\t`});assert.equal(guest.client.roomId,host.client.roomId);
  third.request('joinDuel',{token:code});assert.match(third.error(),/full/);assert.equal(third.client.roomId,priorRoom);
  // Existing members may rejoin idempotently, even when the room is full.
  guest.request('joinDuel',{token:code});assert.equal(guest.messages.at(-1)?.t,'room');
  const room=(lobby as unknown as {duels:Map<string,Room>}).duels.get(code)!;
  room.race={started:true} as Race;
  third.request('joinDuel',{token:code});assert.match(third.error(),/already started/);
  room.race=null;
  host.request('closeRoom',{roomId:host.client.roomId});
  third.request('joinDuel',{token:code});assert.match(third.error(),/invalid or no longer active/);
  assert.equal(third.client.roomId,priorRoom);
  third.request('leaveRoom');
});
