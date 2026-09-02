#include <assert.h>
#include <string.h>

#include "game/asset.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/state.h"

static u8 s_ImageData[16];
static GameImageAssetHeaderWord *s_UploadedImage;
static s32 s_DisplayMask;
static s32 s_RenderOtShift;
static s32 s_DisplaySetups;
static s32 s_CameraMatrixUpdates;
static MATRIX *s_ColorMatrix;
static MATRIX *s_LightMatrix;
static s32 s_BackColor[3];
static s32 s_FarColor[3];
static s32 s_Fog[2];

s32 g_AssetLoadState;
u8 *g_ImageBlockBuffer = s_ImageData;
s32 g_FrameSyncThreshold;
s32 g_MirrorMode;
s32 g_SceneId;
s32 g_SceneTimer;
s32 g_OptionLetterboxHeight;
s32 g_FadeLevel;
s32 g_FadeStep;
s32 g_GameMode;
GameRenderState g_RenderState;
Matrix g_DefaultColorMatrix;
Matrix g_DefaultLightMatrix;
Matrix g_SceneColorMatrix;
Matrix g_SceneLightMatrix;

void SetDispMask(s32 enabled) { s_DisplayMask = enabled; }
void UploadImageAsset(GameImageAssetHeaderWord *asset) {
    s_UploadedImage = asset;
}
void InitRenderState(s32 otShift) { s_RenderOtShift = otShift; }
void SetupDisplay480(s32 r, s32 g, s32 b) {
    assert(r == 0 && g == 0 && b == 0);
    s_DisplaySetups++;
}
void SetCameraRotMatrix(void) { s_CameraMatrixUpdates++; }
void SetColorMatrix(MATRIX *matrix) { s_ColorMatrix = matrix; }
void SetLightMatrix(MATRIX *matrix) { s_LightMatrix = matrix; }
void SetBackColor(long r, long g, long b) {
    s_BackColor[0] = (s32)r;
    s_BackColor[1] = (s32)g;
    s_BackColor[2] = (s32)b;
}
void SetFarColor(long r, long g, long b) {
    s_FarColor[0] = (s32)r;
    s_FarColor[1] = (s32)g;
    s_FarColor[2] = (s32)b;
}
void SetFogNear(long nearDistance, long projection) {
    s_Fog[0] = (s32)nearDistance;
    s_Fog[1] = (s32)projection;
}

static void ResetCalls(void) {
    s_UploadedImage = NULL;
    s_DisplayMask = -1;
    s_RenderOtShift = -1;
    s_DisplaySetups = 0;
    s_CameraMatrixUpdates = 0;
    s_ColorMatrix = NULL;
    s_LightMatrix = NULL;
}

static void TestWaitsForAssets(void) {
    ResetCalls();
    g_AssetLoadState = 1;
    g_SceneId = 99;

    EnterAttractScene();

    assert(s_DisplayMask == 0);
    assert(g_FrameSyncThreshold == 0x80);
    assert(s_UploadedImage == NULL);
    assert(s_DisplaySetups == 0);
    assert(g_SceneId == 99);
}

static void TestInitializesAttractScene(void) {
    ResetCalls();
    memset(&g_DefaultColorMatrix, 0x12, sizeof(g_DefaultColorMatrix));
    memset(&g_DefaultLightMatrix, 0x34, sizeof(g_DefaultLightMatrix));
    memset(&g_RenderState, 0x55, sizeof(g_RenderState));
    g_AssetLoadState = 0;

    EnterAttractScene();

    assert(s_UploadedImage == GetImageAssetHeaderWords(s_ImageData));
    assert(g_MirrorMode == 0);
    assert(s_RenderOtShift == 5);
    assert(s_DisplaySetups == 1);
    assert(g_SceneId == 0x17 && g_SceneTimer == 0);
    assert(memcmp(&g_SceneColorMatrix, &g_DefaultColorMatrix,
                  sizeof(g_SceneColorMatrix)) == 0);
    assert(memcmp(&g_SceneLightMatrix, &g_DefaultLightMatrix,
                  sizeof(g_SceneLightMatrix)) == 0);
    assert(s_ColorMatrix == &g_SceneColorMatrix);
    assert(s_LightMatrix == &g_SceneLightMatrix);
    assert(s_BackColor[0] == 0x20 && s_BackColor[1] == 0x20 &&
           s_BackColor[2] == 0x20);
    assert(s_FarColor[0] == 0 && s_FarColor[1] == 0 && s_FarColor[2] == 0);
    assert(s_Fog[0] == 0x4E20 && s_Fog[1] == 0x140);
    assert(g_RenderState.viewX == 0 && g_RenderState.viewY == 0 &&
           g_RenderState.viewZ == -3520);
    assert(g_RenderState.viewAngleX == 0 &&
           g_RenderState.viewAngleY == 0 &&
           g_RenderState.viewAngleZ == 0);
    assert(s_CameraMatrixUpdates == 1);
    assert(g_OptionLetterboxHeight == 240);
    assert(g_FadeLevel == 256 && g_FadeStep == -8);
    assert(g_GameMode == 0);
}

int main(void) {
    TestWaitsForAssets();
    TestInitializesAttractScene();
    return 0;
}
