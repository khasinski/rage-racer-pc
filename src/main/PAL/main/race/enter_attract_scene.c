#include "game/asset.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/state.h"

enum {
    ATTRACT_SCENE_ID = 0x17,
    ATTRACT_RENDER_OT_SHIFT = 5,
    ATTRACT_VIEW_Z = -3520,
    ATTRACT_LETTERBOX_HEIGHT = 240,
    ATTRACT_INITIAL_FADE = 256,
    ATTRACT_FADE_STEP = -8,
};

static void InitAttractLighting(void) {
    g_SceneColorMatrix = g_DefaultColorMatrix;
    g_SceneLightMatrix = g_DefaultLightMatrix;
    SetColorMatrix(&g_SceneColorMatrix);
    SetLightMatrix(&g_SceneLightMatrix);
    SetBackColor(0x20, 0x20, 0x20);
    SetFarColor(0, 0, 0);
    SetFogNear(0x4E20, 0x140);
}

void EnterAttractScene(void) {
    SetDispMask(0);
    g_FrameSyncThreshold = 0x80;
    if (!AssetLoadCompletedSuccessfully()) {
        return;
    }

    if (!UploadImageAsset(GetImageAssetHeaderWords(g_ImageBlockBuffer),
                          g_ImageBlockSize)) {
        return;
    }
    g_MirrorMode = 0;
    InitRenderState(ATTRACT_RENDER_OT_SHIFT);
    SetupDisplay480(0, 0, 0);
    g_SceneId = ATTRACT_SCENE_ID;
    g_SceneTimer = 0;
    InitAttractLighting();
    g_RenderState.viewX = 0;
    g_RenderState.viewY = 0;
    g_RenderState.viewZ = ATTRACT_VIEW_Z;
    g_RenderState.viewAngleX = 0;
    g_RenderState.viewAngleY = 0;
    g_RenderState.viewAngleZ = 0;
    SetCameraRotMatrix();
    g_OptionLetterboxHeight = ATTRACT_LETTERBOX_HEIGHT;
    g_FadeLevel = ATTRACT_INITIAL_FADE;
    g_GameMode = 0;
    g_FadeStep = ATTRACT_FADE_STEP;
}
