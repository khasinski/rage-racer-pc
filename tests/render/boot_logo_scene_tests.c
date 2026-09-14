#include "game/asset.h"
#include "game/fmv.h"
#include "game/scene.h"
#include "game/state.h"
#include "game/scene_state.h"

#include <limits.h>
#include <stdio.h>

s32 g_AssetLoadState;
static s32 s_assetLoadFailed;
static BootLogo s_boot = {
    .state = BOOT_LOGO_STATE_FADE_IN,
    .holdTimer = BOOT_LOGO_INITIAL_HOLD_FRAMES,
};
s32 g_SceneTimer;
u16 g_PadHeld;

static s32 s_displayMask;
static s32 s_display240Calls;
static s32 s_display480Calls;
static s32 s_endingDraws;
static s32 s_logoDraws;
static s32 s_fmvReturnScene;

BootLogo *SceneRuntimeBootLogo(void) { return &s_boot; }

s32 AssetLoadCompletedSuccessfully(void) {
    return g_AssetLoadState == 0 && !s_assetLoadFailed;
}

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
    s_boot.timer = 9;
    s_displayMask = -1;
    UpdateBootLogoScene();
    CHECK(s_boot.timer == 10 && s_endingDraws == 1);
    CHECK(s_displayMask == -1);
    UpdateBootLogoScene();
    CHECK(s_boot.timer == 11 && s_endingDraws == 2);
    CHECK(s_displayMask == 1);

    s_boot.timer = 110;
    UpdateBootLogoScene();
    CHECK(s_boot.timer == 111 && s_displayMask == 0);
    CHECK(s_display480Calls == 1 && s_logoDraws == 0);

    s_boot.state = BOOT_LOGO_STATE_FADE_IN;
    g_SceneTimer = 248;
    UpdateBootLogoScene();
    CHECK(g_SceneTimer == 256 && s_boot.state == BOOT_LOGO_STATE_FADE_IN);
    CHECK(s_logoDraws == 1);
    UpdateBootLogoScene();
    CHECK(s_boot.state == BOOT_LOGO_STATE_HOLD);

    s_boot.state = BOOT_LOGO_STATE_FADE_IN;
    g_SceneTimer = 251;
    UpdateBootLogoScene();
    CHECK(g_SceneTimer == 256 && s_boot.state == BOOT_LOGO_STATE_FADE_IN);

    s_boot.state = BOOT_LOGO_STATE_HOLD;
    s_boot.holdTimer = -1;
    UpdateBootLogoScene();
    CHECK(s_boot.holdTimer == 0);
    CHECK(s_boot.state == BOOT_LOGO_STATE_FADE_OUT);

    s_boot.holdTimer = 10;
    s_boot.state = BOOT_LOGO_STATE_HOLD;
    g_AssetLoadState = 0;
    s_assetLoadFailed = 1;
    g_PadHeld = 1;
    UpdateBootLogoScene();
    CHECK(s_boot.holdTimer == 9);
    CHECK(s_boot.state == BOOT_LOGO_STATE_HOLD);

    s_assetLoadFailed = 0;
    UpdateBootLogoScene();
    CHECK(s_boot.holdTimer == 0);
    CHECK(s_boot.state == BOOT_LOGO_STATE_FADE_OUT);

    g_PadHeld = 0;
    g_SceneTimer = 8;
    UpdateBootLogoScene();
    CHECK(g_SceneTimer == 0 && s_boot.state == BOOT_LOGO_STATE_START_FMV);
    CHECK(s_display240Calls == 1);

    s_boot.state = BOOT_LOGO_STATE_FADE_OUT;
    g_SceneTimer = 5;
    UpdateBootLogoScene();
    CHECK(g_SceneTimer == 0 && s_boot.state == BOOT_LOGO_STATE_START_FMV);
    CHECK(s_display240Calls == 2);

    g_SceneTimer = 20;
    s_fmvReturnScene = -1;
    UpdateBootLogoScene();
    CHECK(g_SceneTimer == 21 &&
          s_fmvReturnScene == GAME_SCENE_ENTER_TITLE);

    s_boot.state = BOOT_LOGO_STATE_FADE_IN;
    g_SceneTimer = INT_MIN;
    UpdateBootLogoScene();
    CHECK(g_SceneTimer == 8);

    s_boot.state = BOOT_LOGO_STATE_FADE_OUT;
    g_SceneTimer = INT_MAX;
    UpdateBootLogoScene();
    CHECK(g_SceneTimer == 248);

    s_boot.state = BOOT_LOGO_STATE_START_FMV;
    g_SceneTimer = INT_MIN;
    s_fmvReturnScene = -1;
    UpdateBootLogoScene();
    CHECK(g_SceneTimer == 1 && s_fmvReturnScene == -1);

    g_SceneTimer = INT_MAX;
    UpdateBootLogoScene();
    CHECK(g_SceneTimer == INT_MAX &&
          s_fmvReturnScene == GAME_SCENE_ENTER_TITLE);

    s_boot.timer = 111;
    s_boot.state = (BootLogoState)99;
    g_SceneTimer = 123;
    UpdateBootLogoScene();
    CHECK(s_boot.state == BOOT_LOGO_STATE_FADE_IN);
    CHECK(g_SceneTimer == 0);

    s_boot.timer = -1;
    s_boot.state = BOOT_LOGO_STATE_INVALID;
    UpdateBootLogoScene();
    CHECK(s_boot.timer == 1);

    puts("boot logo scene tests passed");
    return 0;
}
