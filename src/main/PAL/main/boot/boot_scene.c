#include "game/prim.h"
#include "game/asset.h"
#include "game/fmv.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render_internal.h"

enum {
    BOOT_ENDING_STILL_FRAMES = 110,
    BOOT_ENDING_STILL_DISPLAY_AT = 10,
    BOOT_LOGO_FADE_LIMIT = 0x100,
    BOOT_LOGO_FADE_STEP = 8,
    BOOT_FMV_START_DELAY = 21,
    TITLE_SCENE_ID = 3,
};

static void AdvanceBootLogoFadeIn(void) {
    if (g_SceneTimer < 0) {
        g_SceneTimer = 0;
    }
    if (g_SceneTimer < BOOT_LOGO_FADE_LIMIT) {
        g_SceneTimer += BOOT_LOGO_FADE_STEP;
        if (g_SceneTimer > BOOT_LOGO_FADE_LIMIT) {
            g_SceneTimer = BOOT_LOGO_FADE_LIMIT;
        }
        return;
    }

    g_BootLogoState = BOOT_LOGO_STATE_HOLD;
}

static void AdvanceBootLogoFadeOut(void) {
    if (g_SceneTimer > BOOT_LOGO_FADE_LIMIT) {
        g_SceneTimer = BOOT_LOGO_FADE_LIMIT;
    }
    if (g_SceneTimer > BOOT_LOGO_FADE_STEP) {
        g_SceneTimer -= BOOT_LOGO_FADE_STEP;
        return;
    }

    g_SceneTimer = 0;
    g_BootLogoState = BOOT_LOGO_STATE_START_FMV;
    SetupDisplay240(0, 0, 0);
}

void UpdateBootLogoScene(void) {
    if (g_BootLogoTimer < 0) {
        g_BootLogoTimer = 0;
    }
    if (g_BootLogoTimer < BOOT_ENDING_STILL_FRAMES) {
        if (g_BootLogoTimer >= BOOT_ENDING_STILL_DISPLAY_AT) {
            SetDispMask(1);
        }
        DrawEndingStill();
        g_BootLogoTimer++;
        return;
    }
    if (g_BootLogoTimer == BOOT_ENDING_STILL_FRAMES) {
        SetDispMask(0);
        SetupDisplay480(0, 0, 0);
        g_BootLogoTimer++;
        return;
    }

    if (g_BootLogoHoldTimer > 0) {
        g_BootLogoHoldTimer--;
        if (AssetLoadCompletedSuccessfully() && g_PadHeld != 0) {
            g_BootLogoHoldTimer = 0;
        }
    } else {
        g_BootLogoHoldTimer = 0;
    }

    switch (g_BootLogoState) {
    case BOOT_LOGO_STATE_INVALID:
        break;
    case BOOT_LOGO_STATE_FADE_IN:
        AdvanceBootLogoFadeIn();
        break;
    case BOOT_LOGO_STATE_HOLD:
        if (g_BootLogoHoldTimer == 0) {
            g_BootLogoState = BOOT_LOGO_STATE_FADE_OUT;
        }
        break;
    case BOOT_LOGO_STATE_FADE_OUT:
        AdvanceBootLogoFadeOut();
        break;
    case BOOT_LOGO_STATE_START_FMV:
        if (g_SceneTimer < 0) {
            g_SceneTimer = 0;
        }
        if (g_SceneTimer < BOOT_FMV_START_DELAY) {
            g_SceneTimer++;
        }
        if (g_SceneTimer >= BOOT_FMV_START_DELAY) {
            BeginIntroFmv(TITLE_SCENE_ID);
        }
        break;
    }

    if (g_BootLogoState != BOOT_LOGO_STATE_START_FMV) {
        DrawBootLogo();
        if ((u32)g_SceneTimer >= BOOT_ENDING_STILL_DISPLAY_AT) {
            SetDispMask(1);
        }
    }
}
