#include "game/prim.h"
#include "game/asset.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render_internal.h"


void UpdateBootLogoScene(void) {
    BootLogoState state;

    if (g_BootLogoTimer < 110) {
        if (g_BootLogoTimer >= 10) {
            SetDispMask(1);
        }
        DrawEndingStill();
        g_BootLogoTimer++;
        return;
    } else if (g_BootLogoTimer == 110) {
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

    state = g_BootLogoState;
    switch (state) {
    case BOOT_LOGO_STATE_INVALID:
        break;
    case BOOT_LOGO_STATE_FADE_IN: {
        u32 sceneTime;

        sceneTime = g_SceneTimer;
        if (sceneTime < 0x100) {
            g_SceneTimer += 8;
        } else {
            g_BootLogoState = BOOT_LOGO_STATE_HOLD;
        }
        break;
    }
    case BOOT_LOGO_STATE_HOLD:
        if (g_BootLogoHoldTimer == 0) {
            g_BootLogoState = BOOT_LOGO_STATE_FADE_OUT;
        }
        break;
    case BOOT_LOGO_STATE_FADE_OUT:
        g_SceneTimer -= 8;
        if (g_SceneTimer == 0) {
            g_BootLogoState = BOOT_LOGO_STATE_START_FMV;
            SetupDisplay240(0, 0, 0);
        }
        break;
    case BOOT_LOGO_STATE_START_FMV: {
        u32 sceneTime;

        g_SceneTimer++;
        sceneTime = g_SceneTimer;
        if (sceneTime >= 21) {
            BeginIntroFmv(3);
        }
        break;
    }
    }

    if (g_BootLogoState != BOOT_LOGO_STATE_START_FMV) {
        u32 sceneTime;

        DrawBootLogo();
        sceneTime = g_SceneTimer;
        if (sceneTime >= 10) {
            SetDispMask(1);
        }
    }
}
