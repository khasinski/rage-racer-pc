#include "game/prim.h"
#include "game/asset.h"
#include "game/fmv.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/scene.h"
#include "game/scene_runtime.h"

enum {
    BOOT_ENDING_STILL_FRAMES = 110,
    BOOT_ENDING_STILL_DISPLAY_AT = 10,
    BOOT_LOGO_FADE_LIMIT = 0x100,
    BOOT_LOGO_FADE_STEP = 8,
    BOOT_FMV_START_DELAY = 21,
};

static void AdvanceBootLogoFadeIn(BootLogo *state) {
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

    state->state = BOOT_LOGO_STATE_HOLD;
}

static void AdvanceBootLogoFadeOut(BootLogo *state) {
    if (g_SceneTimer > BOOT_LOGO_FADE_LIMIT) {
        g_SceneTimer = BOOT_LOGO_FADE_LIMIT;
    }
    if (g_SceneTimer > BOOT_LOGO_FADE_STEP) {
        g_SceneTimer -= BOOT_LOGO_FADE_STEP;
        return;
    }

    g_SceneTimer = 0;
    state->state = BOOT_LOGO_STATE_START_FMV;
    SetupDisplay240(0, 0, 0);
}

void UpdateBootLogoScene(void) {
    BootLogo *state = SceneRuntimeBootLogo();

    if (state->timer < 0) {
        state->timer = 0;
    }
    if (state->timer < BOOT_ENDING_STILL_FRAMES) {
        if (state->timer >= BOOT_ENDING_STILL_DISPLAY_AT) {
            SetDispMask(1);
        }
        DrawEndingStill();
        state->timer++;
        return;
    }
    if (state->timer == BOOT_ENDING_STILL_FRAMES) {
        SetDispMask(0);
        SetupDisplay480(0, 0, 0);
        state->timer++;
        return;
    }

    if (state->holdTimer > 0) {
        state->holdTimer--;
        if (AssetLoadCompletedSuccessfully() && g_PadHeld != 0) {
            state->holdTimer = 0;
        }
    } else {
        state->holdTimer = 0;
    }

    switch (state->state) {
    case BOOT_LOGO_STATE_INVALID:
        break;
    case BOOT_LOGO_STATE_FADE_IN:
        AdvanceBootLogoFadeIn(state);
        break;
    case BOOT_LOGO_STATE_HOLD:
        if (state->holdTimer == 0) {
            state->state = BOOT_LOGO_STATE_FADE_OUT;
        }
        break;
    case BOOT_LOGO_STATE_FADE_OUT:
        AdvanceBootLogoFadeOut(state);
        break;
    case BOOT_LOGO_STATE_START_FMV:
        if (g_SceneTimer < 0) {
            g_SceneTimer = 0;
        }
        if (g_SceneTimer < BOOT_FMV_START_DELAY) {
            g_SceneTimer++;
        }
        if (g_SceneTimer >= BOOT_FMV_START_DELAY) {
            BeginIntroFmv(GAME_SCENE_ENTER_TITLE);
        }
        break;
    default:
        state->state = BOOT_LOGO_STATE_FADE_IN;
        g_SceneTimer = 0;
        break;
    }

    if (state->state != BOOT_LOGO_STATE_START_FMV) {
        DrawBootLogo();
        if ((u32)g_SceneTimer >= BOOT_ENDING_STILL_DISPLAY_AT) {
            SetDispMask(1);
        }
    }
}
