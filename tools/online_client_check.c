#include "online/model.h"
#include "render/session_provider.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void OnlineOpen(void);void OnlineTick(void);void OnlineShutdown(void);
int main(int argc,char**argv){
    if(argc<4){fprintf(stderr,"usage: online-client-check DISC SERVER host|join|arcade|arcade-cycle [seconds] [lifecycle]\n");return 2;}
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER,"dummy");if(!SDL_Init(SDL_INIT_EVENTS|SDL_INIT_AUDIO))return 2;
    int arcade=!strncmp(argv[3],"arcade",6),cycle=!strcmp(argv[3],"arcade-cycle"),firstEpoch=0;
    OnlineSetDisc(argv[1]);OnlineSetServer(argv[2]);OnlineOpen();OnlineActionDo(arcade?OA_ARCADE:OA_GUEST,0,0);
    uint64_t start=SDL_GetTicks();int roomRequested=0,ready=0,started=0,frames=0;unsigned long ticks=0;int racing=0,left=0,results=0;
    int lifecycle=argc>5&&!strcmp(argv[5],"lifecycle");
    uint64_t until=start+(uint64_t)(argc>4?atoi(argv[4]):30)*1000u;
    while(SDL_GetTicks()<until){
        OnlineTick();const OnlineView*v=OnlineViewState();
        if(v->arcadeEpoch&&!firstEpoch)firstEpoch=v->arcadeEpoch;
        if(cycle&&v->arcadeEpoch>firstEpoch&&v->localSeat>=0){printf("ARCADE next round: epoch=%d seat=%d udp=%d\n",v->arcadeEpoch,v->localSeat,v->fastTransport);break;}
        if(!arcade&&v->screen==OS_ROOMS&&!roomRequested){
            if(!strcmp(argv[3],"host")){OnlineActionDo(OA_QUICK_ROOM,0,0);roomRequested=1;}
            else for(int i=0;i<v->count;i++)if(v->rows[i].action==OA_JOIN&&v->rows[i].enabled&&strstr(v->rows[i].value,"Join lobby")){OnlineActionDo(OA_JOIN,v->rows[i].argument,0);roomRequested=1;break;}
        }
        if(v->screen==OS_ROOM){
            if(!ready){for(int i=0;i<v->count;i++)if(v->rows[i].action==OA_READY&&v->rows[i].enabled){OnlineActionDo(OA_READY,0,0);ready=1;break;}}
            if(!started&&!strcmp(argv[3],"host")){
                int players=0;for(int i=0;i<v->count;i++)if(v->rows[i].action==OA_PLAYERS)players=atoi(v->rows[i].value);
                for(int i=0;i<v->count;i++)if(players>=2&&v->rows[i].action==OA_START&&v->rows[i].enabled){OnlineActionDo(OA_START,0,0);started=1;}
            }
        }
        if(v->racing){
            int32_t input[8]={1,0,0,0,256,0,0,0};OnlineSetDriverInput(0,input);racing=1;
            const RenderWorld*w=SessionProviderWorld(16.0f/9.0f);
            if(w){frames++;for(uint32_t i=0;i<w->instanceCount;i++){if(!SessionProviderMesh(&w->instances[i])){fprintf(stderr,"missing mesh %u\n",i);OnlineShutdown();return 1;}}}
            if((unsigned long)v->hud[11]>ticks+100){ticks=(unsigned long)v->hud[11];printf("tick=%lu speed=%d position=%d lap=%d/%d render_frames=%d seat=%d udp=%d epoch=%d\n",ticks,v->hud[7],v->hud[4],v->hud[2],v->hud[3],frames,v->localSeat,v->fastTransport,v->arcadeEpoch);fflush(stdout);}
        }
        if(lifecycle&&v->racing&&ticks>500&&!left){OnlineActionDo(OA_LEAVE_RACE,0,0);left=1;}
        if(v->screen==OS_RESULTS){printf("RESULTS received: %d rows\n",v->count);results=1;OnlineActionDo(OA_BACK,0,0);if(OnlineViewState()->screen!=(arcade?OS_ARCADE:OS_ROOM))return 1;if(!cycle)break;}
        SDL_Delay(4);
    }
    const OnlineView*v=OnlineViewState();printf("native online: screen=%d status=%s racing=%d frames=%d tick=%lu\n",v->screen,v->status,racing,frames,ticks);
    int success=racing&&frames>10&&ticks>100&&(!lifecycle||results)&&(!cycle||(results&&v->arcadeEpoch>firstEpoch&&v->localSeat>=0));
    OnlineShutdown();SDL_Quit();return success?0:1;
}
