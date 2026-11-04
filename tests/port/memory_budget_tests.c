#include "memory_budget.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

static const char *total = "16384", *available = "8192", *profile = "auto";
const char *RuntimeConfigGet(const char *key) {
    if (!strcmp(key, "diagnostics.memory_total_mb")) return total;
    if (!strcmp(key, "diagnostics.memory_available_mb")) return available;
    if (!strcmp(key, "memory.profile")) return profile;
    return NULL;
}
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while (0)
static int SDLCALL Pressure(void *context) {
    (void)context;
    SDL_Event event = {0}; event.type = SDL_EVENT_LOW_MEMORY;
    return SDL_PushEvent(&event) ? 0 : 1;
}
int main(int argc, char **argv) {
    CHECK(argc == 2);
    CHECK(MemoryBudgetClassify(0,0)==RAGE_MEMORY_NORMAL);
    CHECK(MemoryBudgetClassify(2048,0)==RAGE_MEMORY_MINIMUM);
    CHECK(MemoryBudgetClassify(2049,0)==RAGE_MEMORY_LOW);
    CHECK(MemoryBudgetClassify(4096,0)==RAGE_MEMORY_LOW);
    CHECK(MemoryBudgetClassify(4097,0)==RAGE_MEMORY_NORMAL);
    CHECK(MemoryBudgetClassify(16384,512)==RAGE_MEMORY_MINIMUM);
    CHECK(MemoryBudgetClassify(16384,513)==RAGE_MEMORY_LOW);
    CHECK(MemoryBudgetClassify(16384,1024)==RAGE_MEMORY_LOW);
    RageMemoryTier expected=RAGE_MEMORY_NORMAL;
    if(!strcmp(argv[1],"2gb")){total="2048";expected=RAGE_MEMORY_MINIMUM;}
    if(!strcmp(argv[1],"4gb")){total="4096";expected=RAGE_MEMORY_LOW;}
    if(!strcmp(argv[1],"available")){available="256";expected=RAGE_MEMORY_MINIMUM;}
    if(!strcmp(argv[1],"forced_low")){profile="low";expected=RAGE_MEMORY_LOW;}
    if(!strcmp(argv[1],"forced_minimum")){profile="minimum";expected=RAGE_MEMORY_MINIMUM;}
    if(!strcmp(argv[1],"unknown")){total=available="0";}
    if(!strcmp(argv[1],"normal_override")){total="2048";profile="normal";}
    CHECK(MemoryBudgetGet()->tier==expected);
    RagePortConfig config=PortConfigDefaultValues();
    config.renderer=RAGE_RENDERER_CLASSIC;
    config.modernBrightness=1.25f;
    config.modernDrawDistance=2;
    CHECK(MemoryBudgetApplyConfig(&config)==(expected!=RAGE_MEMORY_NORMAL));
    CHECK(config.renderer==RAGE_RENDERER_CLASSIC && config.modernBrightness==1.25f);
    CHECK(MemoryBudgetAllowHd()==(expected==RAGE_MEMORY_NORMAL));
    if(expected!=RAGE_MEMORY_NORMAL){
        CHECK(config.modernInternalScale==(expected==RAGE_MEMORY_MINIMUM?1.5f:2));
        CHECK(config.modernDrawDistance==1 && !config.modernSsao && !config.modernShadows);
        CHECK(config.modernFps==(expected==RAGE_MEMORY_MINIMUM?30:60));
        CHECK(MemoryBudgetApplyConfig(&config)==0);
        config.modernInternalScale=0.5f;config.modernFps=RAGE_MODERN_FPS_LOGIC;
        MemoryBudgetApplyConfig(&config);
        CHECK(config.modernInternalScale==0.5f&&config.modernFps==RAGE_MODERN_FPS_LOGIC);
    }
    CHECK(SDL_Init(SDL_INIT_EVENTS));
    MemoryBudgetStartWatch();
    if(!strcmp(argv[1],"pressure")){
        SDL_Thread *thread=SDL_CreateThread(Pressure,"memory-pressure-test",NULL);
        CHECK(thread);int result=-1;SDL_WaitThread(thread,&result);CHECK(result==0);
        CHECK(MemoryBudgetGet()->tier==RAGE_MEMORY_NORMAL);
        CHECK(MemoryBudgetPoll());
        CHECK(MemoryBudgetGet()->tier==RAGE_MEMORY_MINIMUM&&!MemoryBudgetAllowHd());
        CHECK(!MemoryBudgetPoll());
    }
    MemoryBudgetStopWatch();SDL_Quit();
    return 0;
}
