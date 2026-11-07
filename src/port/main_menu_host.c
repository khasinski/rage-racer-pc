#include "main_menu_ui.h"
#include "game/menu.h"
#include "game/frontend_internal.h"
#include "game/race.h"
#include "game/scene.h"
#include "game/state.h"
#include <SDL3/SDL.h>
#ifndef __ANDROID__
#include "online/online.h"
#include "online/model.h"
#include "host_disc.h"
#include "runtime_config.h"
#endif

static int s_pendingSelection = -1;
static int s_activate;
MainMenuState PortMainMenuState(void) {
    MainMenuState state = {0};
    if (g_SceneId != GAME_SCENE_FRONTEND) return state;
    const Frontend *f = MenuFrontend();
    state.active = f->state == FRONTEND_STATE_MENU_OPENING ||
                   f->state == FRONTEND_STATE_MENU_INPUT ||
                   f->state == FRONTEND_STATE_MENU_EXIT;
    state.interactive = f->state == FRONTEND_STATE_MENU_INPUT;
    state.selected = f->selection;
    state.extraUnlocked = g_ExtraGrandPrixUnlocked != 0;
    state.opacity = f->state == FRONTEND_STATE_MENU_EXIT
        ? 1.0f - (float)f->pulse / 129.0f : 1.0f;
    if (state.opacity < 0) state.opacity = 0;
    return state;
}
void PortMainMenuPointer(int width, int height, float x, float y, int click) {
    MainMenuState state = PortMainMenuState();
    if (!state.interactive) return;
    int item = MainMenuHit(width, height, x, y, state.extraUnlocked);
    if (item < 0) return;
    s_pendingSelection = item;
    s_activate |= click != 0;
}
void PortMainMenuEdge(int last) {
    if (PortMainMenuState().interactive)
        s_pendingSelection = last ? TITLE_MENU_QUIT : TITLE_MENU_GRAND_PRIX;
}
int PortMainMenuTakeInput(int *selection) {
    int result = 0;
    if (s_pendingSelection >= 0) { *selection = s_pendingSelection; result |= 1; }
    if (s_activate) result |= 2;
    s_pendingSelection = -1;
    s_activate = 0;
    return result;
}
void PortMainMenuRequestQuit(void) {
    SDL_Event event;
    SDL_zero(event);
    event.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&event);
}
void PortMainMenuRequestOnline(void) {
#ifndef __ANDROID__
    OnlineSetDisc(HostDiscImagePath());
    const char *server=RuntimeConfigGet("online.server");
    if(server)OnlineSetServer(server);
    OnlineOpen();
#endif
}
