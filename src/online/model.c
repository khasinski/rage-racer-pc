#include "model.h"
#include "core_api.h"
#include "transport.h"
#include "render/session_provider.h"
#include "render/speed_units_poc.h"
#include "SDL3/SDL.h"
#include "yyjson.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

typedef struct Settings {char name[64];int classIndex,course,reverse,laps,rivals,maxPlayers;} Settings;
typedef struct Room {int id,players,status;Settings settings;} Room;
typedef struct Member {int id,variant,manual,tire,ready,host,spectator,online,ping;char name[48];} Member;
static OnlineView view;
static OnlineTransport *transport;
static char disc[4096],server[512]="https://rageracer.polanka.ovh",name[21]="Racer",password[201],token[256],chat[201];
static int loaded,connected,roomId,duel,roomStatus,roomPage,memberCount,roomCount,host,go,gotFrame,offeredRoom;
static int requestedSpeedUnits;static uint32_t hudUnitsRevision;
static Room rooms[ONLINE_ROOMS];static Member members[ONLINE_MEMBERS];static Settings settings;
static int wantArcade,leavingArcade,pendingSeat=-1,nextVariant=-1,courseVote;static uint32_t controlAt;
static OnlineScreen chatReturn=OS_ROOMS;
static char userName[48],invite[64],joinCode[64];static char *editing;static size_t editCapacity;
static uint64_t lastClock,lastStep,previousStep,retryAt;static double accumulator,simTime;
static int inputHeld,inputStick,inputThrottle,inputBrake,inputAnalog;
static int rawInputActive;
static int32_t rawInput[8];
static uint8_t inputHistory[4][40];static unsigned inputHistoryCount;
static uint32_t newestTick;static int margins[12],marginCount,marginAt;static uint32_t settleSeq;
static int clockStarted,clockObservation;static uint64_t assetEpoch,frameSerial;
static RenderWorld world;static RenderMeshInstance instances[4096];
static RageRuntimeCachedMesh meshes[64];static unsigned meshCount;
static SDL_AudioStream *audio;
static void rows(void);
static void disconnect(int closeUI);
static void fail(const char*text){SDL_strlcpy(view.status,text,sizeof(view.status));}
static void appendChat(const char *from,const char *text){
    if(view.chatCount==ONLINE_CHAT_LINES){memmove(view.messages,view.messages+1,(ONLINE_CHAT_LINES-1)*sizeof(view.messages[0]));--view.chatCount;}
    char *line=view.messages[view.chatCount++];
    snprintf(line,256,"%s%s%s",from,*from?": ":"",text);view.chatOffset=0;
}
static int integer(yyjson_val*v,const char*k){return (int)yyjson_get_int(yyjson_obj_get(v,k));}
static int boolean(yyjson_val*v,const char*k){return yyjson_is_true(yyjson_obj_get(v,k));}
static const char *string(yyjson_val*v,const char*k){const char*s=yyjson_get_str(yyjson_obj_get(v,k));return s?s:"";}
static int sendObject(yyjson_mut_doc*d){size_t n=0;char*s=yyjson_mut_write(d,0,&n);int ok=s&&OnlineTransportSend(transport,0,s,n);free(s);yyjson_mut_doc_free(d);return ok;}
static yyjson_mut_doc *message(const char*t){yyjson_mut_doc*d=yyjson_mut_doc_new(NULL);if(!d)return NULL;yyjson_mut_val*v=yyjson_mut_obj(d);yyjson_mut_doc_set_root(d,v);yyjson_mut_obj_add_str(d,v,"t",t);return d;}
static int sendSimple(const char*t){yyjson_mut_doc*d=message(t);return d&&sendObject(d);}
static void sendNumber(const char*t,const char*k,int v){yyjson_mut_doc*d=message(t);if(!d)return;yyjson_mut_obj_add_int(d,yyjson_mut_doc_get_root(d),k,v);sendObject(d);}
static void sendBool(const char*t,const char*k,int v){yyjson_mut_doc*d=message(t);if(!d)return;yyjson_mut_obj_add_bool(d,yyjson_mut_doc_get_root(d),k,v!=0);sendObject(d);}
static void sendText(const char*t,const char*k,const char*v){yyjson_mut_doc*d=message(t);if(!d)return;yyjson_mut_obj_add_str(d,yyjson_mut_doc_get_root(d),k,v);sendObject(d);}
static Member *me(void){for(int i=0;i<memberCount;i++)if(members[i].id==view.userId)return &members[i];return NULL;}
static void readSettings(yyjson_val*v,Settings*s){SDL_strlcpy(s->name,string(v,"name"),sizeof(s->name));s->classIndex=integer(v,"classIndex");s->course=integer(v,"course");s->reverse=boolean(v,"reverse");s->laps=integer(v,"laps");s->rivals=boolean(v,"rivals");s->maxPlayers=integer(v,"maxPlayers");}
static void sendSettings(void){
    yyjson_mut_doc*d=message("updateRoom");if(!d)return;yyjson_mut_val*v=yyjson_mut_obj(d),*r=yyjson_mut_doc_get_root(d);
    yyjson_mut_obj_add_val(d,r,"settings",v);yyjson_mut_obj_add_str(d,v,"name",settings.name);
    yyjson_mut_obj_add_int(d,v,"classIndex",settings.classIndex);yyjson_mut_obj_add_int(d,v,"course",settings.course);
    yyjson_mut_obj_add_int(d,v,"laps",settings.laps);yyjson_mut_obj_add_int(d,v,"maxPlayers",settings.maxPlayers);
    yyjson_mut_obj_add_bool(d,v,"rivals",settings.rivals!=0);yyjson_mut_obj_add_bool(d,v,"reverse",settings.reverse!=0);sendObject(d);
}
static void setCar(int variant,int manual,int tire){yyjson_mut_doc*d=message("setCar");if(!d)return;yyjson_mut_val*v=yyjson_mut_doc_get_root(d);
    yyjson_mut_obj_add_int(d,v,"variant",variant);yyjson_mut_obj_add_bool(d,v,"manual",manual!=0);yyjson_mut_obj_add_int(d,v,"tire",tire);sendObject(d);}
static void releaseMeshes(void){for(unsigned i=0;i<meshCount;i++)RuntimeCachedMeshRelease(&meshes[i]);meshCount=0;}
static void stopRace(void){SessionProviderSet(NULL);view.racing=view.raceReady=0;go=gotFrame=0;inputHistoryCount=0;memset(rawInput,0,sizeof(rawInput));rawInputActive=0;releaseMeshes();if(audio){SDL_DestroyAudioStream(audio);audio=NULL;}}
static const RageRuntimeCachedMesh *mesh(void*ctx,const RenderMeshInstance*i){
    (void)ctx;unsigned key=i->assetKey&0xffffu;
    for(unsigned n=0;n<meshCount;n++)if(meshes[n].assetKey==key&&meshes[n].assetSet==i->assetSet)return &meshes[n];
    if(meshCount>=64)return NULL;size_t size=0;const void*bytes=on_mesh_bytes((unsigned)i->assetSet,key,&size);if(!bytes)return NULL;
    RageRuntimeCachedMesh*c=&meshes[meshCount];memset(c,0,sizeof(*c));c->assetKey=key;c->assetSet=i->assetSet;
    if(!RuntimeCachedMeshAdopt(c,bytes,size,NULL,NULL))return NULL;++meshCount;return c;
}
static int material(void*ctx,const RenderMeshInstance*i,uint32_t m,uint8_t variant,RageRenderMaterial*d,void**pixels,uint32_t*w,uint32_t*h){
    (void)ctx;uint8_t*p=SDL_malloc(256u*256u*4u);if(!p)return 0;
    unsigned originalEntity=(i->assetKey>>16)&255u;
    if(!on_material((unsigned)i->assetSet,i->assetKey&0xffffu,originalEntity,m,variant,i->hasCarPaint,i->carPaintColor1,i->carPaintColor2,p,256u*256u*4u)){SDL_free(p);return 0;}
    RenderMaterialDefault(d);d->roughness=i->assetSet==RAGE_RENDER_ASSET_TERRAIN?0.96f:0.5f;
    *pixels=p;*w=*h=256;return 1;
}
static int sky(void*ctx,void**pixels,uint32_t*w,uint32_t*h){
    (void)ctx;int width=rw_sky_width(),height=rw_sky_height();if(width<=0||height<=0||width>2048||height>1024)return 0;
    uint8_t*p=SDL_malloc((size_t)width*(size_t)height*4u);if(!p)return 0;if(!rw_decode_sky(p)){SDL_free(p);return 0;}
    *pixels=p;*w=(uint32_t)width;*h=(uint32_t)height;return 1;
}
static uint64_t revision(void*ctx){(void)ctx;return (assetEpoch<<32)^rw_palette_hash()^(uint32_t)rw_texture_page();}
static const RenderWorld *scene(void*ctx,float aspect){
    (void)ctx;if(!view.racing||!view.raceReady)return NULL;
    double interval=lastStep>previousStep?(double)(lastStep-previousStep):40.0;
    float fraction=(float)((simTime+accumulator-(double)lastStep)/interval);if(fraction<0)fraction=0;if(fraction>1)fraction=1;
    if(on_prepare(aspect,fraction)<0)return NULL;
    if(on_camera_size()!=(int)sizeof(RenderCamera)||on_instance_size()!=(int)sizeof(RenderMeshInstance)){fail("Online renderer ABI mismatch.");return NULL;}
    RenderWorldInit(&world,instances,4096);RenderWorldBeginFrame(&world,++frameSerial);
    if(!on_copy_camera(&world.camera,(int)sizeof(world.camera))||!on_copy_light(&world.light,(int)sizeof(world.light)))return NULL;
    world.camera.inCarView=0;world.hasCamera=1;
    int n=on_world_count();if(n<0||n>4096)return NULL;
    for(int at=0;at<n;at++){
        RenderMeshInstance item;if(!on_copy_instance(at,&item,(int)sizeof(item)))return NULL;
        unsigned seat=item.entity;
        unsigned appearance=item.assetSet==RAGE_RENDER_ASSET_MODEL_BANK?seat&255u:0;
        item.assetKey=(item.assetKey&0xffffu)|(appearance<<16)|0x80000000u;
        unsigned flags=item.flags;item.flags&=~((1u<<7)|(1u<<8)|(1u<<10));if(flags&(1u<<7))item.flags|=RAGE_RENDER_INSTANCE_SHADOW_ONLY;
        item.flags|=RAGE_SESSION_INSTANCE;
        if(view.localSeat>=0){if(item.entity==(uint32_t)view.localSeat)item.entity=RAGE_RENDER_PLAYER_CAR_ENTITY;else if(item.entity==RAGE_RENDER_PLAYER_CAR_ENTITY)item.entity=(uint32_t)view.localSeat;}
        if(!RenderWorldSubmitMesh(&world,&item))return NULL;
    }
    return &world;
}
static int tachometer(void *ctx,RageRetailTachometer *frame){
    (void)ctx;if(!view.raceReady||view.localSeat<0)return 0;
    uint32_t current=rw_hud_atlas_revision();
    if(requestedSpeedUnits&&current!=hudUnitsRevision){
        uint8_t snapshot[RAGE_HUD_UNIT_BYTES];RageHudUnitDiff diff;
        int variant=on_hud_unit_snapshot(snapshot,sizeof(snapshot))-1;
        int model=variant>=0?rw_car_model(variant):-1;
        if(model>=0&&RageSpeedUnitsRetailDiff((unsigned)model,&diff)&&
           RageHudUnitDiffApply(&diff,(unsigned)model,requestedSpeedUnits==RAGE_UNITS_MPH,snapshot))
            on_hud_apply_units(snapshot,sizeof(snapshot),requestedSpeedUnits==RAGE_UNITS_MPH);
        hudUnitsRevision=rw_hud_atlas_revision();
    }
    const int32_t *words=rw_tachometer();const uint8_t *atlas=rw_hud_atlas();
    if(!words||!atlas||!words[RAGE_TACHO_VISIBLE])return 0;
    memcpy(frame->words,words,sizeof(frame->words));frame->atlas=atlas;
    frame->revision=(assetEpoch<<32)|rw_hud_atlas_revision();return 1;
}
static void registerProvider(void){RageSessionProvider p={.world=scene,.mesh=mesh,.material=material,.sky=sky,.revision=revision,.tachometer=tachometer};SessionProviderSet(&p);}
static void connectAs(const char*action){
    if(transport){if(!OnlineTransportDone(transport)){fail("Finishing previous connection...");return;}OnlineTransportFree(transport);transport=NULL;}
    if(!loaded){on_native_client();if(!disc[0]||!rw_load_disc(disc)){fail("Cannot load the selected game image for online play.");return;}loaded=1;}
    yyjson_mut_doc*d=yyjson_mut_doc_new(NULL);if(!d)return;yyjson_mut_val*v=yyjson_mut_obj(d);yyjson_mut_doc_set_root(d,v);
    if(!strcmp(action,"resume"))yyjson_mut_obj_add_str(d,v,"token",token);
    else{if(name[0])yyjson_mut_obj_add_str(d,v,"name",name);if(strcmp(action,"guest"))yyjson_mut_obj_add_str(d,v,"password",password);}
    char*json=yyjson_mut_write(d,0,NULL);transport=json?OnlineTransportStart(server,action,json):NULL;
    if(json){memset(json,0,strlen(json));free(json);}yyjson_mut_doc_free(d);memset(password,0,sizeof(password));
    connected=0;view.screen=OS_CONNECTING;view.panel=1;view.selected=0;fail(transport?"Connecting...":"Invalid server address or connection could not start.");
    if(!transport)view.screen=OS_PROFILE;
}
static void decodeLogo(const char*text,uint8_t*out,size_t capacity){
    unsigned bits=0,count=0;size_t used=0;memset(out,0,capacity);
    const char*alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    for(const char*p=text;*p&&*p!='=';p++){const char*c=strchr(alphabet,*p);if(!c)return;bits=(bits<<6)|(unsigned)(c-alphabet);count+=6;if(count>=8){count-=8;if(used>=capacity)return;out[used++]=(uint8_t)(bits>>count);}}
}
static void raceStart(yyjson_val*root){
    stopRace();readSettings(yyjson_obj_get(root,"settings"),&settings);
    view.arcade=boolean(root,"arcade");view.arcadeEpoch=integer(root,"epoch");pendingSeat=-1;controlAt=0;view.takeoverSeconds=0;
    yyjson_val*seats=yyjson_obj_get(root,"seats");int humans=integer(root,"humans"),local=integer(root,"localSeat");
    int32_t setup[36]={0};if(humans<1||humans>12||local>=humans||local< -1){sendBool("loaded","ok",0);fail("Invalid race setup.");return;}
    for(int i=0;i<humans;i++){
        yyjson_val*seat=yyjson_arr_get(seats,(size_t)i);if(!yyjson_is_obj(seat)){sendBool("loaded","ok",0);return;}
        setup[i*3]=integer(seat,"variant");setup[i*3+1]=boolean(seat,"manual");setup[i*3+2]=integer(seat,"tire");
        yyjson_val*paint=yyjson_obj_get(seat,"paint");int a=-1,b=-1;
        if(yyjson_arr_size(paint)==2){a=(int)yyjson_get_int(yyjson_arr_get(paint,0));b=(int)yyjson_get_int(yyjson_arr_get(paint,1));}
        uint8_t logo[2080];const char*text=string(seat,"logo");if(*text)decodeLogo(text,logo,sizeof(logo));
        rw_set_look(i,a,b,*text?logo:NULL,string(seat,"name"));
    }
    int ok=rw_start_net_race(settings.classIndex,settings.course,settings.reverse,settings.laps,settings.rivals,humans,setup,local);
    if(view.arcade){yyjson_mut_doc *d=message("arcadeLoaded");if(d){yyjson_mut_val *v=yyjson_mut_doc_get_root(d);yyjson_mut_obj_add_int(d,v,"epoch",view.arcadeEpoch);yyjson_mut_obj_add_bool(d,v,"ok",ok!=0);sendObject(d);}}
    else sendBool("loaded","ok",ok);
    if(!ok){fail("Could not prepare the online race.");view.screen=view.arcade?OS_ARCADE:OS_ROOM;return;}
    if(view.arcade)rw_set_view_seat(integer(root,"viewSeat"));
    view.localSeat=local;view.racing=1;view.panel=0;view.screen=OS_RACE;view.selected=0;
    lastClock=SDL_GetTicks();lastStep=previousStep=0;accumulator=simTime=0;newestTick=0;clockStarted=0;marginAt=marginCount=0;settleSeq=0;
    ++assetEpoch;registerProvider();rw_audio_enable(SDL_getenv("PSYZ_HEADLESS")==NULL);
    if(SDL_getenv("PSYZ_HEADLESS")==NULL){SDL_AudioSpec spec={SDL_AUDIO_S16,2,44100};audio=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,NULL,NULL);if(audio)SDL_ResumeAudioStreamDevice(audio);}
    fail(view.arcade?"Preparing your car...":"Waiting for the other drivers...");
}
static void handleText(const uint8_t*bytes,size_t size){
    yyjson_doc*d=yyjson_read((const char*)bytes,size,0);if(!d)return;yyjson_val*r=yyjson_doc_get_root(d);const char*t=string(r,"t");
    if(!strcmp(t,"arcadeLeft")){leavingArcade=0;view.arcade=wantArcade=0;stopRace();view.screen=OS_ROOMS;view.panel=1;yyjson_doc_free(d);return;}
    if(leavingArcade&&(!strncmp(t,"arcade",6)||(!strcmp(t,"raceStart")&&boolean(r,"arcade")))){yyjson_doc_free(d);return;}
    if(!strcmp(t,"welcome")){
        yyjson_val*u=yyjson_obj_get(r,"user");view.userId=integer(u,"id");SDL_strlcpy(userName,string(u,"name"),sizeof(userName));
        if(strcmp(rw_disc_id(),string(r,"discId"))){fail("Your game image differs from the server's release.");OnlineTransportStop(transport);token[0]=0;view.screen=OS_PROFILE;}
        else{connected=1;OnlineTransportUseRtc(transport);view.screen=roomId?OS_ROOM:OS_ROOMS;view.selected=0;fail("Connected. Choose or create a room.");if(wantArcade)sendSimple("joinArcade");}
    }else if(!strcmp(t,"rooms")){
        roomCount=0;yyjson_val*list=yyjson_obj_get(r,"rooms");size_t idx,max;yyjson_val*item;
        yyjson_arr_foreach(list,idx,max,item){if(roomCount>=ONLINE_ROOMS)break;Room*room=&rooms[roomCount++];room->id=integer(item,"id");room->players=integer(item,"players");room->status=strcmp(string(item,"status"),"lobby")!=0;readSettings(yyjson_obj_get(item,"settings"),&room->settings);}
    }else if(!strcmp(t,"room")){
        yyjson_val*room=yyjson_obj_get(r,"room");memberCount=0;host=0;
        if(!yyjson_is_obj(room)){roomId=0;offeredRoom=0;if(!view.racing)view.screen=OS_ROOMS;view.selected=0;}
        else{
            roomId=integer(room,"id");roomStatus=strcmp(string(room,"status"),"lobby")!=0;readSettings(yyjson_obj_get(room,"settings"),&settings);
            duel=yyjson_is_obj(yyjson_obj_get(room,"duel"));SDL_strlcpy(invite,string(yyjson_obj_get(room,"duel"),"token"),sizeof(invite));
            yyjson_val*list=yyjson_obj_get(room,"members");size_t idx,max;yyjson_val*item;
            yyjson_arr_foreach(list,idx,max,item){if(memberCount>=ONLINE_MEMBERS)break;Member*m=&members[memberCount++];m->id=integer(item,"userId");m->variant=integer(item,"variant");m->manual=boolean(item,"manual");m->tire=integer(item,"tire");m->ready=boolean(item,"ready");m->host=boolean(item,"host");m->spectator=boolean(item,"spectator");m->online=boolean(item,"online");m->ping=-1;SDL_strlcpy(m->name,string(item,"name"),sizeof(m->name));}
            Member*m=me();host=m&&m->host;
            if(m&&!m->spectator&&m->variant<0&&offeredRoom!=roomId){for(int model=0;model<13;model++){int variant=rw_class_car(settings.classIndex,model);if(variant>=0){setCar(variant,!rw_car_automatic(variant),0);offeredRoom=roomId;break;}}}
            if(!view.racing&&view.screen!=OS_RESULTS&&view.screen!=OS_PLAYERS)view.screen=OS_ROOM;
        }
    }else if(!strcmp(t,"arcadeState")){
        view.arcade=1;wantArcade=1;view.nextRaceSeconds=(integer(r,"nextInMs")+999)/1000;view.nextCourse=integer(r,"nextCourse");view.nextClass=integer(r,"nextClass");
        memberCount=0;yyjson_val *list=yyjson_obj_get(r,"members"),*item;size_t idx,max;
        yyjson_arr_foreach(list,idx,max,item){if(memberCount>=ONLINE_MEMBERS)break;Member *m=&members[memberCount++];memset(m,0,sizeof(*m));m->id=integer(item,"userId");m->spectator=integer(item,"seat")<0;m->online=boolean(item,"online");m->ping=-1;SDL_strlcpy(m->name,string(item,"name"),sizeof(m->name));}
        if(!view.racing&&view.screen!=OS_RESULTS&&view.screen!=OS_CHAT){view.screen=OS_ARCADE;view.panel=1;}
    }else if(!strcmp(t,"arcadeControl")){
        if(view.arcade&&integer(r,"epoch")==view.arcadeEpoch){pendingSeat=integer(r,"seat");controlAt=(uint32_t)integer(r,"activateAt");view.lateJoin=boolean(r,"late");if(pendingSeat>=0&&pendingSeat<12)rw_set_view_seat(pendingSeat);}
    }else if(!strcmp(t,"arcadeChat")){
        view.chatCount=0;yyjson_val *list=yyjson_obj_get(r,"lines"),*item;size_t idx,max;
        yyjson_arr_foreach(list,idx,max,item)appendChat(string(item,"from"),string(item,"text"));
    }else if(!strcmp(t,"raceStart"))raceStart(r);
    else if(!strcmp(t,"raceGo")){go=1;view.status[0]=0;}
    else if(!strcmp(t,"latency")){
        yyjson_val*pings=yyjson_obj_get(r,"latency");for(int i=0;i<memberCount;i++){char key[24];snprintf(key,sizeof(key),"%d",members[i].id);yyjson_val*p=yyjson_obj_get(pings,key);members[i].ping=yyjson_is_num(p)?(int)yyjson_get_int(p):-1;}
    }else if(!strcmp(t,"chat")){appendChat(string(r,"from"),string(r,"text"));SDL_strlcpy(view.status,view.messages[view.chatCount-1],sizeof(view.status));}
    else if(!strcmp(t,"error")){fail(string(r,"message"));}
    else if(!strcmp(t,"raceEvent")){yyjson_val*e=yyjson_obj_get(r,"event");snprintf(view.status,sizeof(view.status),"%.40s: %.40s",string(e,"name"),string(e,"kind"));}
    else if(!strcmp(t,"results")){
        stopRace();view.screen=OS_RESULTS;view.panel=1;view.selected=0;view.count=0;SDL_strlcpy(view.title,"Race results",sizeof(view.title));
        yyjson_val*list=yyjson_obj_get(r,"results");size_t idx,max;yyjson_val*item;
        yyjson_arr_foreach(list,idx,max,item){if(view.count>=ONLINE_ROWS-1)break;OnlineRow*row=&view.rows[view.count++];memset(row,0,sizeof(*row));snprintf(row->label,sizeof(row->label),"%d. %.48s",integer(item,"place"),string(item,"name"));int ms=integer(item,"timeMs");if(ms>=0)snprintf(row->value,sizeof(row->value),"%s %d:%02d.%03d",string(item,"status"),ms/60000,ms/1000%60,ms%1000);else SDL_strlcpy(row->value,boolean(item,"joinedLate")?"Joined in progress":string(item,"status"),sizeof(row->value));}
        OnlineRow*row=&view.rows[view.count++];memset(row,0,sizeof(*row));SDL_strlcpy(row->label,view.arcade?"Arcade lobby / chat / next race":"Return to room / race again",sizeof(row->label));row->action=OA_BACK;row->enabled=1;
    }
    yyjson_doc_free(d);
}
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void handleFrame(const uint8_t*p,size_t size){
    if(!view.racing||size<6)return;
    unsigned base=6;
    if(p[0]==4||p[0]==5){if((p[0]==5)!=!!view.arcade||size<10||u32(p+1)!=(uint32_t)view.arcadeEpoch)return;base=10;}
    else if(view.arcade||p[0]!=2)return;
    uint32_t tick=u32(p+base-5);unsigned players=p[base-1];size_t header=base+players*12u;
    if(players>12||size<header||tick<=newestTick)return;
    uint32_t sequence=0,applied=0;int margin=0;
    if(view.localSeat>=0){if((unsigned)view.localSeat>=players)return;const uint8_t*ack=p+base+(unsigned)view.localSeat*12;sequence=u32(ack);applied=u32(ack+4);margin=(int32_t)u32(ack+8);}
    if(rw_apply_frame(p+header,(int)(size-header),sequence,applied)==INT32_MIN){fail("Server race frame is incompatible.");return;}
    if(!gotFrame)lastClock=SDL_GetTicks();
    newestTick=tick;gotFrame=1;
    if(view.arcade&&pendingSeat>=0&&pendingSeat<12&&tick>=controlAt){
        if(on_local_seat(pendingSeat)){view.localSeat=pendingSeat;inputHistoryCount=0;view.takeoverSeconds=0;lastClock=SDL_GetTicks();accumulator=0;clockStarted=clockObservation=marginCount=marginAt=0;settleSeq=0;fail(view.lateJoin?"Joined in progress; full race time is not ranked.":"");}
        pendingSeat=-1;
    }
    view.takeoverSeconds=pendingSeat>=0&&controlAt>tick?(int)((controlAt-tick+49)/50):0;
    if(view.localSeat>=0&&sequence>=settleSeq){margins[marginAt++%12]=margin;if(marginCount<12)++marginCount;clockObservation=1;}
}
static void sampleTick(void){
    rw_set_pad(view.panel?0:inputHeld,inputStick,view.panel?0:inputThrottle,view.panel?0:inputBrake,inputAnalog);
    if(rawInputActive){int32_t words[8];memcpy(words,rawInput,sizeof(words));if(view.panel){memset(words,0,sizeof(words));words[0]=1;}on_set_input(words);rawInput[6]=rawInput[7]=0;}
    if(view.localSeat>=0){int32_t*words=rw_take_input();uint8_t packet[161],latest[40];packet[0]=1;
        uint32_t values[10]={rw_input_seq(),rw_input_tick()};for(int i=0;i<8;i++)values[i+2]=(uint32_t)words[i];
        for(unsigned i=0;i<10;i++)for(unsigned b=0;b<4;b++)latest[i*4+b]=(uint8_t)(values[i]>>(b*8));
        if(inputHistoryCount==4)memmove(inputHistory,inputHistory+1,3*sizeof(inputHistory[0]));else ++inputHistoryCount;
        memcpy(inputHistory[inputHistoryCount-1],latest,sizeof(latest));memcpy(packet+1,inputHistory,inputHistoryCount*40u);
        OnlineTransportSend(transport,1,packet,1+inputHistoryCount*40u);
    }
    if(rw_tick()>=0)view.raceReady=1;simTime+=20;
    if(rw_last_tick_stepped()){previousStep=lastStep;lastStep=(uint64_t)simTime;}
    if(audio){int frames=rw_audio_take();if(frames>0&&SDL_GetAudioStreamQueued(audio)<44100*4)SDL_PutAudioStreamData(audio,rw_audio_data(),frames*4);}
}
static void add(const char*label,const char*value,OnlineAction action,int arg,int enabled,int edit){
    if(view.count>=ONLINE_ROWS)return;OnlineRow*r=&view.rows[view.count++];memset(r,0,sizeof(*r));SDL_strlcpy(r->label,label,sizeof(r->label));if(value)SDL_strlcpy(r->value,value,sizeof(r->value));r->action=action;r->argument=arg;r->enabled=enabled;r->editable=edit;
}
static void rows(void){
    if(view.screen==OS_RESULTS)return;view.count=0;char value[256];
    view.fastTransport=OnlineTransportFast(transport);
    SDL_strlcpy(view.title,view.screen==OS_PROFILE?"Online":view.screen==OS_ROOMS?"Online rooms":view.screen==OS_PLAYERS?"Players":"Online duel",sizeof(view.title));
    SDL_strlcpy(view.subtitle,connected?userName:"Jump into a race. No account needed.",sizeof(view.subtitle));
    if(view.screen==OS_PROFILE){
        add("Play online","Join the next race",OA_ARCADE,0,1,0);
        add("Play with friends","Rooms & invitations",OA_GUEST,0,1,0);
        add("Profile & settings","",OA_PROFILE_OPTIONS,0,1,0);
        add("Back to game","",OA_BACK,0,1,0);
    }else if(view.screen==OS_PROFILE_OPTIONS){
        SDL_strlcpy(view.title,"Profile & settings",sizeof(view.title));
        add("Name",name,OA_NAME,0,1,1);
        add("Account","Optional",OA_ACCOUNT,0,1,0);
        add("Connection settings","",OA_CONNECTION,0,1,0);
        add("Back","",OA_BACK,0,1,0);
    }else if(view.screen==OS_ACCOUNT){
        SDL_strlcpy(view.title,"Account",sizeof(view.title));
        SDL_strlcpy(view.subtitle,"Log in to use your existing racer profile.",sizeof(view.subtitle));
        add("Name",name,OA_NAME,0,1,1);char hidden[24];size_t n=strlen(password);if(n>20)n=20;memset(hidden,'*',n);hidden[n]=0;add("Password",hidden,OA_PASSWORD,0,1,1);
        add("Log in","",OA_LOGIN,0,1,0);add("Create account","",OA_REGISTER,0,1,0);add("Back","",OA_BACK,0,1,0);
    }else if(view.screen==OS_CONNECTION){
        SDL_strlcpy(view.title,"Connection settings",sizeof(view.title));
        SDL_strlcpy(view.subtitle,"The default server is ready to play.",sizeof(view.subtitle));
        add("Server",server,OA_SERVER,0,1,1);add("Back","",OA_BACK,0,1,0);
    }else if(view.screen==OS_CONNECTING){add("Connecting...","",OA_NONE,0,0,0);add("Cancel","",OA_BACK,0,1,0);}
    else if(view.screen==OS_ROOMS){
        add("Online Arcade","Auto-join / continuous racing",OA_ARCADE,0,1,0);
        add("Create room","",OA_QUICK_ROOM,0,1,0);add("Create duel","Same car for both drivers",OA_DUEL,0,1,0);
        add("Join duel: code",joinCode,OA_JOIN_DUEL,0,1,1);
        int first=roomPage*6;if(first>=roomCount)first=roomPage=0;
        for(int i=first;i<roomCount&&i<first+6;i++){snprintf(value,sizeof(value),"%d/%d  %s",rooms[i].players,rooms[i].settings.maxPlayers,rooms[i].status?"Watch race":"Join lobby");add(rooms[i].settings.name,value,OA_JOIN,rooms[i].id,1,0);}
        if(!roomCount)add("No open rooms yet","Create one above",OA_NONE,0,0,0);
        if(roomCount>6)add("More rooms","Left / Right",OA_NEXT_ROOMS,0,1,0);add("Disconnect","",OA_BACK,0,1,0);
    }else if(view.screen==OS_ROOM){
        SDL_strlcpy(view.title,settings.name,sizeof(view.title));Member*m=me();int editable=host&&!roomStatus&&!duel;
        snprintf(value,sizeof(value),"%d connected%s",memberCount,duel?" / shared car":"");add("Players / ping",value,OA_PLAYERS,0,1,0);
        if(duel)add("Copy invitation code",invite,OA_COPY_INVITE,0,invite[0]!=0,0);
        else{snprintf(value,sizeof(value),"%d",settings.maxPlayers);add("Grid size",value,OA_MAX_PLAYERS,0,editable,0);}
        const char*courses[]={"The Mythical Coast","Over Pass City","Lakeside Gate","The Extreme Oval"};add("Course",settings.course>=0&&settings.course<4?courses[settings.course]:"Unknown",OA_COURSE,0,editable,0);
        snprintf(value,sizeof(value),"%d",settings.classIndex+1);add("Class",value,OA_CLASS,0,editable,0);snprintf(value,sizeof(value),"%d",settings.laps);add("Laps",value,OA_LAPS,0,editable,0);
        add("Direction",settings.reverse?"Reverse":"Forward",OA_REVERSE,0,editable,0);add("CPU rivals",settings.rivals?"On":"Off",OA_RIVALS,0,editable,0);
        if(m){int model=m->variant>=0?rw_car_model(m->variant):-1;snprintf(value,sizeof(value),"%s G%d",model>=0?rw_car_name(model):"Choose car",m->variant>=0?rw_car_grade(m->variant)+1:0);add("Car",value,OA_CAR,0,!roomStatus&&!duel&&!m->spectator,0);
            if(!duel)add("Role",m->spectator?"Spectator":"Driver",OA_SPECTATOR,0,!roomStatus,0);
            add("Gearbox",m->manual?"Manual":"Automatic",OA_GEARBOX,0,!roomStatus&&!m->spectator,0);
            snprintf(value,sizeof(value),"%d",m->tire+1);if(duel)add("Tires",value,OA_TIRE,0,!roomStatus&&!m->spectator,0);
            add("Ready",m->ready?"Yes":"No",OA_READY,0,!roomStatus&&!m->spectator&&m->variant>=0,0);
        }
        snprintf(value,sizeof(value),"%d messages",view.chatCount);add("Lobby chat",value,OA_OPEN_CHAT,0,1,0);
        if(roomStatus)add("Watch race","",OA_WATCH,0,1,0);
        else if(host){int ready=memberCount>0;for(int i=0;i<memberCount;i++)if(!members[i].spectator&&(!members[i].ready||members[i].variant<0||!members[i].online))ready=0;add("Start race","",OA_START,0,ready,0);}
        add("Leave room","",OA_LEAVE_ROOM,0,1,0);
    }else if(view.screen==OS_PLAYERS){for(int i=0;i<memberCount&&i<ONLINE_ROWS-1;i++){Member*m=&members[i];snprintf(value,sizeof(value),"%s  %s  %d ms",m->host?"Host":"",m->online?(m->ready?"Ready":"Connected"):"Offline",m->ping<0?0:m->ping);add(m->name,value,OA_NONE,0,0,0);}add("Back","",OA_BACK,0,1,0);}
    else if(view.screen==OS_RACE){add("Resume race","",OA_RESUME,0,1,0);if(view.arcade)add("Arcade lobby","Next car / course vote / chat",OA_ARCADE_LOBBY,0,1,0);add("Settings","",OA_SETTINGS,0,1,0);add("Players / ping","",OA_PLAYERS,0,1,0);add("Chat","",OA_OPEN_CHAT,0,1,0);add(view.arcade?"Leave Online Arcade":"Leave race","",view.arcade?OA_LEAVE_ARCADE:OA_LEAVE_RACE,0,1,0);}
    else if(view.screen==OS_ARCADE){
        SDL_strlcpy(view.title,"Online Arcade",sizeof(view.title));
        snprintf(value,sizeof(value),"%d people / CPU fills 12 cars",memberCount);add("Community",value,OA_PLAYERS,0,1,0);
        if(view.racing)add("Return to the race",view.localSeat>=0?"Driving":"Waiting to take over CPU",OA_RESUME,0,1,0);
        else{snprintf(value,sizeof(value),"%d seconds",view.nextRaceSeconds);add("Next automatic start",value,OA_NONE,0,0,0);}
        snprintf(value,sizeof(value),"%s",nextVariant>=0?rw_car_name(rw_car_model(nextVariant)):"Automatic selection");add("Car for next race",value,OA_ARCADE_CAR,0,1,0);
        const char *courses[]={"The Mythical Coast","Over Pass City","Lakeside Gate","The Extreme Oval"};add("Vote next course",courses[view.nextCourse>=0&&view.nextCourse<4?view.nextCourse:0],OA_ARCADE_VOTE,0,1,0);
        snprintf(value,sizeof(value),"%d messages",view.chatCount);add("Community chat",value,OA_OPEN_CHAT,0,1,0);
        add("Leave Online Arcade","",OA_LEAVE_ARCADE,0,1,0);
    }else if(view.screen==OS_CHAT){
        SDL_strlcpy(view.title,view.arcade?"Arcade community chat":"Lobby chat",sizeof(view.title));
        int end=view.chatCount-view.chatOffset,start=end>8?end-8:0;
        for(int i=start;i<end;i++){add(view.messages[i],"",OA_NONE,0,0,0);}
        if(!view.chatCount)add("No messages yet","",OA_NONE,0,0,0);
        add("Write message",chat,OA_CHAT,0,1,1);
        if(view.chatCount>8)add("Message history","Left: older / Right: newer",OA_CHAT_SCROLL,0,1,0);
        add("Back","",OA_BACK,0,1,0);
    }
    if(view.selected>=view.count)view.selected=view.count?view.count-1:0;
}
void OnlineSetDisc(const char*path){if(!view.active)SDL_strlcpy(disc,path?path:"",sizeof(disc));}
void OnlineSetServer(const char*url){if(!view.active&&url)SDL_strlcpy(server,url,sizeof(server));}
void OnlineSetSpeedUnits(const char*units){if(!view.active){requestedSpeedUnits=units&&!strcmp(units,"mph")?RAGE_UNITS_MPH:units&&!strcmp(units,"kmh")?RAGE_UNITS_KMH:0;hudUnitsRevision=0;}}
const OnlineView *OnlineViewState(void){rows();return &view;}
void OnlineOpen(void){if(view.active)return;view.active=1;view.panel=1;view.screen=OS_PROFILE;view.selected=0;view.status[0]=0;rows();}
int OnlineActive(void){return view.active;}
int OnlineRaceActive(void){return view.racing;}
int OnlinePanelOpen(void){return view.panel;}
void OnlineSelect(int row){rows();if(row>=0&&row<view.count)view.selected=row;}
void OnlineSetInput(int held,int stick,int throttle,int brake,int analog){inputHeld=held;inputStick=stick;inputThrottle=throttle;inputBrake=brake;inputAnalog=analog;rawInputActive=0;}
int OnlineForceFeedback(int32_t values[8]){return view.racing&&connected&&!view.panel&&on_feedback(values);}
void OnlineEffectsVolume(float gain){if(audio)SDL_SetAudioStreamGain(audio,gain);}
void OnlineSetDriverInput(int held,const int32_t words[8]){int up=rawInput[6],down=rawInput[7];memcpy(rawInput,words,sizeof(rawInput));rawInput[6]|=up;rawInput[7]|=down;inputHeld=held;inputStick=inputThrottle=inputBrake=inputAnalog=0;rawInputActive=1;}
static void disconnect(int closeUI){stopRace();connected=0;token[0]=0;roomId=memberCount=offeredRoom=wantArcade=view.arcade=0;OnlineTransportStop(transport);if(loaded){on_release();loaded=0;}view.screen=OS_PROFILE;view.panel=1;view.active=!closeUI;view.editing=0;editing=NULL;}
void OnlineActionDo(OnlineAction action,int arg,int direction){
    Member*m=me();int step=direction<0?-1:1;
    if(action==OA_PROFILE_OPTIONS||action==OA_ACCOUNT||action==OA_CONNECTION){view.screen=action==OA_PROFILE_OPTIONS?OS_PROFILE_OPTIONS:action==OA_ACCOUNT?OS_ACCOUNT:OS_CONNECTION;view.selected=0;return;}
    if(action==OA_BACK&&(view.screen==OS_PROFILE_OPTIONS||view.screen==OS_ACCOUNT||view.screen==OS_CONNECTION)){view.screen=view.screen==OS_PROFILE_OPTIONS?OS_PROFILE:OS_PROFILE_OPTIONS;view.selected=0;return;}
    if(action==OA_SERVER||action==OA_NAME||action==OA_PASSWORD||action==OA_CHAT||action==OA_JOIN_DUEL){editing=action==OA_SERVER?server:action==OA_NAME?name:action==OA_PASSWORD?password:action==OA_JOIN_DUEL?joinCode:chat;editCapacity=action==OA_SERVER?sizeof(server):action==OA_NAME?sizeof(name):action==OA_PASSWORD?sizeof(password):action==OA_JOIN_DUEL?sizeof(joinCode):sizeof(chat);view.editing=(int)action;return;}
    if(action==OA_GUEST||action==OA_LOGIN||action==OA_REGISTER){connectAs(action==OA_GUEST?"guest":action==OA_LOGIN?"login":"register");return;}
    if(action==OA_ARCADE){leavingArcade=0;view.arcade=wantArcade=1;if(!connected){connectAs("guest");return;}sendSimple("joinArcade");view.screen=OS_ARCADE;view.panel=1;return;}
    if(action==OA_BACK){if(view.screen==OS_CHAT){view.screen=chatReturn;view.selected=0;return;}if(view.screen==OS_RESULTS){view.screen=view.arcade?OS_ARCADE:roomId?OS_ROOM:OS_ROOMS;view.selected=0;return;}if(view.screen==OS_PLAYERS){view.screen=view.arcade?OS_ARCADE:view.racing?OS_RACE:OS_ROOM;view.selected=0;return;}if(view.arcade&&view.screen==OS_ARCADE&&view.racing){view.panel=0;view.screen=OS_RACE;return;}disconnect(view.screen==OS_PROFILE);return;}
    if(!connected)return;
    switch(action){
    case OA_LEAVE_ARCADE:sendSimple("leaveArcade");leavingArcade=1;stopRace();view.arcade=wantArcade=0;pendingSeat=-1;controlAt=0;view.screen=OS_ROOMS;view.panel=1;view.selected=0;break;
    case OA_ARCADE_LOBBY:view.screen=OS_ARCADE;view.panel=1;view.selected=0;break;
    case OA_OPEN_CHAT:chatReturn=view.screen;view.screen=OS_CHAT;view.panel=1;view.selected=0;break;
    case OA_CHAT_SCROLL:view.chatOffset+=step<0?8:-8;if(view.chatOffset<0)view.chatOffset=0;if(view.chatOffset>view.chatCount-1)view.chatOffset=view.chatCount-1;break;
    case OA_ARCADE_CAR:for(int i=0;i<32;i++){nextVariant=(nextVariant+step+32)%32;if(rw_car_allowed(view.nextClass,nextVariant)){sendNumber("arcadeCar","variant",nextVariant);break;}}break;
    case OA_ARCADE_VOTE:courseVote=(view.nextCourse+step+4)%4;if(courseVote==3&&view.nextClass<2)courseVote=step>0?0:2;sendNumber("arcadeVote","course",courseVote);break;
    case OA_QUICK_ROOM:sendSimple("quickRoom");break;
    case OA_DUEL:sendSimple("createDuel");break;
    case OA_COPY_INVITE:if(invite[0]){SDL_SetClipboardText(invite);fail("Invitation code copied.");}break;
    case OA_SPECTATOR:if(m&&!duel)sendBool("setSpectator","spectator",!m->spectator);break;
    case OA_MAX_PLAYERS:if(host&&!duel){settings.maxPlayers=(settings.maxPlayers+step+11)%12+1;sendSettings();}break;
    case OA_JOIN:sendNumber("joinRoom","roomId",arg);break;
    case OA_READY:if(m)sendBool("setReady","ready",!m->ready);break;
    case OA_START:sendSimple("startRace");break;
    case OA_LEAVE_ROOM:sendSimple("leaveRoom");break;
    case OA_LEAVE_RACE:sendSimple("leaveRace");stopRace();view.screen=OS_ROOM;view.panel=1;break;
    case OA_WATCH:sendSimple("watchRace");break;
    case OA_RESUME:view.panel=0;view.screen=OS_RACE;break;
    case OA_PLAYERS:view.screen=OS_PLAYERS;view.panel=1;view.selected=0;break;
    case OA_NEXT_ROOMS:roomPage=(roomPage+step+(roomCount+5)/6)%((roomCount+5)/6);break;
    case OA_CAR:if(m&&!duel){int variant=m->variant;for(int i=0;i<32;i++){variant=(variant+step+32)%32;if(rw_car_allowed(settings.classIndex,variant)){setCar(variant,m->manual||!rw_car_automatic(variant),m->tire);break;}}}break;
    case OA_GEARBOX:if(m)setCar(m->variant,!m->manual||!rw_car_automatic(m->variant),m->tire);break;
    case OA_TIRE:if(m)setCar(m->variant,m->manual,(m->tire+step+5)%5);break;
    case OA_COURSE:if(host&&!duel){settings.course=(settings.course+step+4)%4;if(settings.course==3&&settings.classIndex<2)settings.course=step>0?0:2;sendSettings();}break;
    case OA_CLASS:if(host&&!duel){settings.classIndex=(settings.classIndex+step+6)%6;if(settings.classIndex<2&&settings.course==3)settings.course=0;sendSettings();}break;
    case OA_LAPS:if(host&&!duel){settings.laps=(settings.laps+step+5)%6+1;sendSettings();}break;
    case OA_REVERSE:if(host&&!duel){settings.reverse=!settings.reverse;sendSettings();}break;
    case OA_RIVALS:if(host&&!duel){settings.rivals=!settings.rivals;sendSettings();}break;
    default:break;
    }
}
void OnlineText(const char*text){if(!editing||!text)return;size_t have=strlen(editing),n=strlen(text);if(n<editCapacity-have)memcpy(editing+have,text,n+1);}
void OnlineBackspace(void){if(!editing)return;size_t n=strlen(editing);if(!n)return;do{--n;}while(n&&((unsigned char)editing[n]&0xc0)==0x80);editing[n]=0;}
void OnlineFinishEditing(void){if(view.editing==OA_CHAT&&chat[0]){sendText("chat","text",chat);chat[0]=0;}if(view.editing==OA_JOIN_DUEL&&joinCode[0])sendText("joinDuel","token",joinCode);view.editing=0;editing=NULL;}
void OnlinePauseMenu(void){if(view.editing){OnlineFinishEditing();return;}if(view.racing){view.panel=!view.panel;view.screen=OS_RACE;view.selected=0;}else OnlineActionDo(OA_BACK,0,0);}
void OnlineTick(void){
    if(!view.active)return;static uint8_t packet[65537];int type;size_t size;
    while(transport&&OnlineTransportPoll(transport,&type,packet,sizeof(packet),&size)){
        if(type==ONLINE_TEXT)handleText(packet,size);else if(type==ONLINE_FRAME)handleFrame(packet,size);
        else if(type==ONLINE_AUTH){yyjson_doc*d=yyjson_read((char*)packet,size,0);if(d){SDL_strlcpy(token,string(yyjson_doc_get_root(d),"token"),sizeof(token));yyjson_doc_free(d);}}
        else if(type==ONLINE_REPLACED){token[0]=0;retryAt=0;connected=0;stopRace();view.panel=1;view.screen=OS_PROFILE;fail((char*)packet);}
        else if(type==ONLINE_ERROR||type==ONLINE_CLOSED){fail((char*)packet);if(token[0])retryAt=SDL_GetTicks()+1000;connected=0;view.panel=1;view.screen=OS_PROFILE;}
    }
    if(transport&&OnlineTransportDone(transport)){OnlineTransportFree(transport);transport=NULL;}
    if(!transport&&!connected&&token[0]&&retryAt&&SDL_GetTicks()>=retryAt){retryAt=0;connectAs("resume");}
    if(view.racing&&gotFrame&&connected){
        uint64_t now=SDL_GetTicks();double delta=(double)(now-lastClock);lastClock=now;if(delta>250)delta=250;accumulator+=delta;
        if(view.localSeat>=0&&marginCount&&clockObservation){clockObservation=0;int low=margins[0];for(int i=1;i<marginCount;i++)if(margins[i]<low)low=margins[i];
            if(!clockStarted){clockStarted=1;marginCount=marginAt=0;}
            else if(low<0||low>10){accumulator+=(2-low)*20;settleSeq=rw_input_seq()+1;marginCount=marginAt=0;}
            else if(low<2)accumulator+=1;else if(low>3)accumulator-=1;
        }
        int steps=0;while(accumulator>=20&&steps++<16){sampleTick();accumulator-=20;}
        int32_t*h=rw_hud();if(h)memcpy(view.hud,h,sizeof(view.hud));
    }
    rows();
}
void OnlineShutdown(void){disconnect(1);if(transport){OnlineTransportFree(transport);transport=NULL;}}
