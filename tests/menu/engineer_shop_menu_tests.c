#include "common.h"
#include "game/menu.h"

#include <stdio.h>

s32 g_EngineSpecStep;

static s32 s_engineStep;
static s32 s_enginePhase;

void DrawCarEngineSpec(s32 step, s32 phase) {
    s_engineStep = step;
    s_enginePhase = phase;
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
    g_EngineSpecStep = 100;
    CHECK(DrawEngineerShopScreen(0) == 0);

    CHECK(DrawEngineerShopScreen(600) == MENU_FADE_MAX);
    CHECK(s_engineStep == 0);
    CHECK(s_enginePhase == MENU_FADE_MAX / 4);

    CHECK(DrawEngineerShopScreen(-MENU_FADE_MAX) == 0);
    CHECK(s_engineStep == MENU_FADE_MAX * MENU_FADE_MAX / 2048);
    CHECK(s_enginePhase == 0);

    puts("engineer shop menu tests passed");
    return 0;
}
