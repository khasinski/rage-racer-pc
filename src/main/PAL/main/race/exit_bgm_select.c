#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/render.h"
#include "game/state.h"


void ExitBgmSelect(void) {
    if (g_AssetLoadState == 0) {
        g_FadeStep = 4;
    }

    if (g_FadeStep < 0) {
        g_FadeLevel += g_FadeStep;
        if (g_FadeLevel < 0) {
            g_FadeLevel = 0;
            g_FadeStep = 0;
        }
        DrawFullscreenFadeTile(g_FadeLevel, 0x49);
    } else if (g_FadeStep > 0) {
        g_FadeLevel += g_FadeStep;
        DrawFullscreenFadeTile(g_FadeLevel, 0x49);
        if (g_FadeLevel >= 0x101) {
            g_SceneId = 0x16;
        }
    }

    DrawProportionalText(0x5E, 0x72, g_TextNowLoading, 0x7812);
}
