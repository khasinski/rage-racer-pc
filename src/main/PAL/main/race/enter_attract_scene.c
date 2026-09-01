#include "game/asset.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/state.h"

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
    if (g_AssetLoadState != 0) {
        return;
    }

    UploadImageAsset(g_ImageBlockBuffer);
    g_MirrorMode = 0;
    InitRenderState(5);
    SetupDisplay480(0, 0, 0);
    g_SceneId = 0x17;
    g_SceneTimer = 0;
    InitAttractLighting();
    g_RenderState.viewX = 0;
    g_RenderState.viewY = 0;
    g_RenderState.viewZ = -3520;
    g_RenderState.viewAngleX = 0;
    g_RenderState.viewAngleY = 0;
    g_RenderState.viewAngleZ = 0;
    SetCameraRotMatrix();
    g_OptionLetterboxHeight = 0xF0;
    g_FadeLevel = 0x100;
    g_GameMode = 0;
    g_FadeStep = -8;
}
