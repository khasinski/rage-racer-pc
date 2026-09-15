#include "common.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/race.h"

static MenuWidgets s_menuWidgets;

#include <limits.h>
#include <stdio.h>
#include <string.h>

extern s32 g_MenuScreen;

void MenuActivateScreen(s32 screen) {
    g_MenuScreen = screen;
}

static CarEntry s_cars[GAME_CAR_COUNT];
CarEntry *g_CarTable = s_cars;
u32 g_CarModelSlot;
s32 g_MenuScreen;
s32 g_PlayerCarIndex;
s32 g_UiScriptProgress;
RaceSession g_RaceSession;
static CarBrowse s_browse;
CarBrowse *MenuCarBrowse(void) { return &s_browse; }
int CustomRaceModelCount(int classIndex) {
    (void)classIndex;
    return 0;
}

static s32 s_installCalls;
static s32 s_namePlateCalls;
static s32 s_carViewCalls;
static s32 s_lightBurstStep;

s32 ActivateShowroomCarModel(s32 slot) {
    (void)slot;
    s_installCalls++;
    return 1;
}
void DrawCarNamePlate(MenuWidgets *widgets) {
    s32 step = widgets->carNameStep;
    s32 model = widgets->carNameModel;
    (void)step;
    (void)model;
    s_namePlateCalls++;
}
void DrawMenuCarView(void) { s_carViewCalls++; }
void DrawMenuLightBurst(MenuWidgets *widgets, s32 step) {
    (void)widgets;
    s_lightBurstStep = step;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,         \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    memset(s_cars, 0, sizeof(s_cars));
    s_cars[1].enabled = 1;
    s_cars[4].enabled = 1;
    s_cars[10].enabled = 1;
    g_PlayerCarIndex = 6;
    UpdateOwnedCarNeighbours(&s_browse);
    CHECK(s_browse.previous == 4);
    CHECK(s_browse.next == 10);

    g_PlayerCarIndex = 1;
    UpdateOwnedCarNeighbours(&s_browse);
    CHECK(s_browse.previous == -1);
    CHECK(s_browse.next == 4);

    g_PlayerCarIndex = 10;
    UpdateOwnedCarNeighbours(&s_browse);
    CHECK(s_browse.previous == 4);
    CHECK(s_browse.next == -1);

    g_PlayerCarIndex = INT_MIN;
    UpdateOwnedCarNeighbours(&s_browse);
    CHECK(s_browse.previous == -1 && s_browse.next == -1);
    g_PlayerCarIndex = INT_MAX;
    UpdateOwnedCarNeighbours(&s_browse);
    CHECK(s_browse.previous == -1 && s_browse.next == -1);
    g_CarTable = NULL;
    g_PlayerCarIndex = 4;
    UpdateOwnedCarNeighbours(&s_browse);
    CHECK(s_browse.previous == -1 && s_browse.next == -1);
    g_CarTable = s_cars;
    g_PlayerCarIndex = 10;

    g_UiScriptProgress = 99;
    EnterCarSelectScreen();
    CHECK(g_MenuScreen == 4);
    CHECK(g_UiScriptProgress == 0 && s_installCalls == 1);
    CHECK(s_namePlateCalls == 1 && s_carViewCalls == 1);
    CHECK(s_lightBurstStep == -9);

    puts("car select state tests passed");
    return 0;
}

MenuWidgets *MenuWidgetState(void) { return &s_menuWidgets; }
