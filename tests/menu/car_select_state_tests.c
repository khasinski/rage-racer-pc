#include "common.h"
#include "game/car.h"
#include "game/menu.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

static CarEntry s_cars[GAME_CAR_COUNT];
CarEntry *g_CarTable = s_cars;
s32 g_CarNamePlateStep;
s32 g_MenuAltLayout;
s32 g_MenuAltLayoutSetting;
s32 g_MenuPlateCarIndex;
s32 g_MenuScreen;
s16 g_NextOwnedCarIndex;
s32 g_PlayerCarIndex;
s16 g_PrevOwnedCarIndex;
s32 g_UiScriptProgress;

static s32 s_installCalls;
static s32 s_namePlateCalls;
static s32 s_carViewCalls;
static s32 s_lightBurstStep;

void ActivateShowroomCarModel(void) { s_installCalls++; }
void DrawCarNamePlate(s32 step, s32 model, s32 grade) {
    (void)step;
    (void)model;
    (void)grade;
    s_namePlateCalls++;
}
void DrawMenuCarView(void) { s_carViewCalls++; }
void DrawMenuLightBurst(s32 step) { s_lightBurstStep = step; }

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
    UpdateOwnedCarNeighbours();
    CHECK(g_PrevOwnedCarIndex == 4);
    CHECK(g_NextOwnedCarIndex == 10);

    g_PlayerCarIndex = 1;
    UpdateOwnedCarNeighbours();
    CHECK(g_PrevOwnedCarIndex == -1);
    CHECK(g_NextOwnedCarIndex == 4);

    g_PlayerCarIndex = 10;
    UpdateOwnedCarNeighbours();
    CHECK(g_PrevOwnedCarIndex == 4);
    CHECK(g_NextOwnedCarIndex == -1);

    g_PlayerCarIndex = INT_MIN;
    UpdateOwnedCarNeighbours();
    CHECK(g_PrevOwnedCarIndex == -1 && g_NextOwnedCarIndex == -1);
    g_PlayerCarIndex = INT_MAX;
    UpdateOwnedCarNeighbours();
    CHECK(g_PrevOwnedCarIndex == -1 && g_NextOwnedCarIndex == -1);
    g_CarTable = NULL;
    g_PlayerCarIndex = 4;
    UpdateOwnedCarNeighbours();
    CHECK(g_PrevOwnedCarIndex == -1 && g_NextOwnedCarIndex == -1);
    g_CarTable = s_cars;
    g_PlayerCarIndex = 10;

    g_MenuAltLayoutSetting = 3;
    g_UiScriptProgress = 99;
    EnterCarSelectScreen();
    CHECK(g_MenuAltLayout == 3 && g_MenuScreen == 4);
    CHECK(g_UiScriptProgress == 0 && s_installCalls == 1);
    CHECK(s_namePlateCalls == 1 && s_carViewCalls == 1);
    CHECK(s_lightBurstStep == -9);

    puts("car select state tests passed");
    return 0;
}
