#include "online.h"
void OnlineOpen(void){}
int OnlineActive(void){return 0;}
int OnlineRaceActive(void){return 0;}
int OnlinePanelOpen(void){return 0;}
void OnlineTick(void){}
void OnlineSampleInput(void){}
int OnlineEvent(const SDL_Event*e,SDL_Window*w){(void)e;(void)w;return 0;}
int OnlinePaint(OverlaySurface*s,int w,int h,OverlayLayout*l){(void)s;(void)w;(void)h;(void)l;return 0;}
void OnlineShutdown(void){}
void OnlineSmokeTick(void){}
