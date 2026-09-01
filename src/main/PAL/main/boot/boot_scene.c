#include "game/prim.h"
#include "game/asset.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render_internal.h"

enum {
    BOOT_ENDING_STILL_FRAMES = 110,
    BOOT_ENDING_STILL_DISPLAY_AT = 10,
    BOOT_LOGO_FADE_LIMIT = 0x100,
    BOOT_LOGO_FADE_STEP = 8,
    BOOT_FMV_START_DELAY = 21,
};

void UpdateBootLogoScene(void) {
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

    if (g_BootLogoHoldTimer != 0) {
        g_BootLogoHoldTimer--;
        if ((g_AssetLoadState == 0) && (g_PadHeld != 0)) {
            g_BootLogoHoldTimer = 0;
        }
    }

    switch (g_BootLogoState) {
    case BOOT_LOGO_STATE_INVALID:
        break;
    case BOOT_LOGO_STATE_FADE_IN:
        if ((u32)g_SceneTimer < BOOT_LOGO_FADE_LIMIT) {
            g_SceneTimer += BOOT_LOGO_FADE_STEP;
        } else {
            g_BootLogoState = BOOT_LOGO_STATE_HOLD;
        }
        break;
    case BOOT_LOGO_STATE_HOLD:
        if (g_BootLogoHoldTimer == 0) {
            g_BootLogoState = BOOT_LOGO_STATE_FADE_OUT;
        }
        break;
    case BOOT_LOGO_STATE_FADE_OUT:
        g_SceneTimer -= BOOT_LOGO_FADE_STEP;
        if (g_SceneTimer == 0) {
            g_BootLogoState = BOOT_LOGO_STATE_START_FMV;
            SetupDisplay240(0, 0, 0);
        }
        break;
    case BOOT_LOGO_STATE_START_FMV:
        g_SceneTimer++;
        if ((u32)g_SceneTimer >= BOOT_FMV_START_DELAY) {
            BeginIntroFmv(3);
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
