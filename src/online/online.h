#ifndef RAGE_NATIVE_ONLINE_H
#define RAGE_NATIVE_ONLINE_H
#include <stdint.h>
#include <SDL3/SDL.h>
#include "port/overlay_raster.h"
/* Main-thread UI and simulation. Socket ownership is confined to transport. */
void OnlineOpen(void);
int OnlineActive(void);
int OnlineRaceActive(void);
int OnlinePanelOpen(void);
void OnlineSampleInput(void);
void OnlineTick(void);
int OnlineEvent(const SDL_Event *event,SDL_Window *window);
int OnlinePaint(OverlaySurface *,int width,int height,OverlayLayout *);
void OnlineShutdown(void);
/* Explicit diagnostic automation; inactive without hooks.online. */
void OnlineSmokeTick(void);
#endif
