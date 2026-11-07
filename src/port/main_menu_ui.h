#ifndef RAGE_MAIN_MENU_UI_H
#define RAGE_MAIN_MENU_UI_H
#include "overlay_raster.h"
#include "rage/title_menu.h"

typedef struct MainMenuState {
    int active, interactive, selected, extraUnlocked;
    float opacity;
} MainMenuState;
typedef struct MainMenuRect { float x, y, w, h; } MainMenuRect;
const char *MainMenuLabel(int item);
MainMenuRect MainMenuRow(int width, int height, int item, int extraUnlocked);
int MainMenuHit(int width, int height, float x, float y, int extraUnlocked);
int MainMenuPaint(OverlaySurface *surface, int width, int height,
                  const MainMenuState *state);
MainMenuState PortMainMenuState(void);
void PortMainMenuPointer(int width, int height, float x, float y, int click);
void PortMainMenuEdge(int last);
/* Bits: 1 selection changed, 2 activate. Input is consumed once. */
int PortMainMenuTakeInput(int *selection);
void PortMainMenuRequestQuit(void);
void PortMainMenuRequestOnline(void);
#endif
