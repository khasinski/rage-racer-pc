#include "game/asset.h"
#include "game/fmv.h"
#include "game/state.h"

#include <stdio.h>

s32 g_AssetLoadState;
s32 g_BootLogoHoldTimer;
BootLogoState g_BootLogoState;
s32 g_BootLogoTimer;
s32 g_SceneTimer;
u16 g_PadHeld;

static s32 s_displayMask;
static s32 s_display240Calls;
static s32 s_display480Calls;
static s32 s_endingDraws;
static s32 s_logoDraws;
static s32 s_fmvReturnScene;

void SetDispMask(s32 enabled) {
    s_displayMask = enabled;
}

void SetupDisplay240(s32 red, s32 green, s32 blue) {
    (void)red;
    (void)green;
    (void)blue;
    s_display240Calls++;
}

void SetupDisplay480(s32 red, s32 green, s32 blue) {
    (void)red;
    (void)green;
    (void)blue;
    s_display480Calls++;
}

void DrawEndingStill(void) {
    s_endingDraws++;
}

void DrawBootLogo(void) {
    s_logoDraws++;
}

void BeginIntroFmv(s32 returnScene) {
    s_fmvReturnScene = returnScene;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__,   \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    g_BootLogoTimer = 9;
    s_displayMask = -1;
    UpdateBootLogoScene();
    CHECK(g_BootLogoTimer == 10 && s_endingDraws == 1);
    CHECK(s_displayMask == -1);
    UpdateBootLogoScene();
    CHECK(g_BootLogoTimer == 11 && s_endingDraws == 2);
    CHECK(s_displayMask == 1);

    g_BootLogoTimer = 110;
    UpdateBootLogoScene();
    CHECK(g_BootLogoTimer == 111 && s_displayMask == 0);
    CHECK(s_display480Calls == 1 && s_logoDraws == 0);

    g_BootLogoState = BOOT_LOGO_STATE_FADE_IN;
    g_SceneTimer = 248;
    UpdateBootLogoScene();
    CHECK(g_SceneTimer == 256 && g_BootLogoState == BOOT_LOGO_STATE_FADE_IN);
    CHECK(s_logoDraws == 1);
    UpdateBootLogoScene();
    CHECK(g_BootLogoState == BOOT_LOGO_STATE_HOLD);

    g_BootLogoState = BOOT_LOGO_STATE_FADE_IN;
    g_SceneTimer = 251;
    UpdateBootLogoScene();
    CHECK(g_SceneTimer == 256 && g_BootLogoState == BOOT_LOGO_STATE_FADE_IN);

    g_BootLogoState = BOOT_LOGO_STATE_HOLD;
    g_BootLogoHoldTimer = -1;
    UpdateBootLogoScene();
    CHECK(g_BootLogoHoldTimer == 0);
    CHECK(g_BootLogoState == BOOT_LOGO_STATE_FADE_OUT);

    g_BootLogoHoldTimer = 10;
    g_AssetLoadState = 0;
    g_PadHeld = 1;
    UpdateBootLogoScene();
    CHECK(g_BootLogoHoldTimer == 0);
    CHECK(g_BootLogoState == BOOT_LOGO_STATE_FADE_OUT);

    g_PadHeld = 0;
    g_SceneTimer = 8;
    UpdateBootLogoScene();
    CHECK(g_SceneTimer == 0 && g_BootLogoState == BOOT_LOGO_STATE_START_FMV);
    CHECK(s_display240Calls == 1);

    g_BootLogoState = BOOT_LOGO_STATE_FADE_OUT;
    g_SceneTimer = 5;
    UpdateBootLogoScene();
    CHECK(g_SceneTimer == 0 && g_BootLogoState == BOOT_LOGO_STATE_START_FMV);
    CHECK(s_display240Calls == 2);

    g_SceneTimer = 20;
    s_fmvReturnScene = -1;
    UpdateBootLogoScene();
    CHECK(g_SceneTimer == 21 && s_fmvReturnScene == 3);

    puts("boot logo scene tests passed");
    return 0;
}
