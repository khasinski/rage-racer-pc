#include "game/prim.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/track.h"

void DrawBgmSelectBar(void) {
    u8 *base;
    s32 tileW;
    s32 tileH;
    s32 temp;
    u8 *next;

    base = (u8 *)GamePrimaryOrderingTable(1);
    next = RENDER_PRIM_CURSOR_AS(u8);
    temp = (g_BgmSelectCursor == 0) ? 0x3FEC : 0x3FEF;
    tileW = 0x14;
    tileH = 0x10;

    next = GameQueueSprite(base, next, 0x20, 0xC1, tileW, tileH, 0, 0, temp);
    temp = (g_BgmSelectCursor == 1) ? 0x3FEC : 0x3FEF;
    next = GameQueueSprite(base, next, 0x36, 0xC1, tileW, tileH, tileW, 0, temp);
    temp = (g_BgmSelectCursor == 2) ? 0x3FEC : 0x3FEF;
    next = GameQueueSprite(base, next, 0x4C, 0xC1, tileW, tileH, 0x28, 0, temp);

    if (g_BgmRandomLabelTimer != 0) {
        g_BgmRandomLabelTimer--;
        temp = 0x10;
    } else {
        temp = g_BgmSelectTrack * 12 + 0x1C;
    }

    next = GameQueueSprite(base, next, 0x64, 0xC2, 0xBA, 0xC, 0, temp, 0x3FED);
    next = GameQueueSprite(base, next, 0x62, 0xC0, 0xBE, 0x10, 0x3C, 0, 0x3FEE);
    next = GameQueueTileTrans(base, next, 0x14, 0xB8, 0x118, 0x20, 0, 0, 0);
    RENDER_PRIM_CURSOR_AS(u8) = QueueDrawModePrim(base, next, 0xB);
}

void UpdateBgmSelect(void) {
    UpdateBgmSelectPlayback();

    if (g_SceneTimer == 2) SetDispMask(1);
    if (g_FadeStep == 0) {
        UpdateBgmSelectInput();
    } else {
        DrawFullscreenFadeTile(g_FadeLevel, 0x49);
        g_FadeLevel += g_FadeStep;
        if (g_FadeLevel >= 256) {
            RequestOptionScreenAssets();
            g_BgmSelectStep = BGM_SELECT_STEP_EXIT;
            g_FadeLevel = 256;
            g_FadeStep = -4;
        }
    }

    if (g_BgmSelectShowUi != 0) DrawBgmSelectBar();
    g_AnimTimer++;
    g_CameraCarIndex = CycleBgmSelectCameraCar(0xff, g_CameraCarIndex);
    UpdateAttractCars();
    RequestTrackTexturePage(g_Cars[g_CameraCarIndex].trackSection);
    UpdateCamera(g_CameraViewMode, (GameRenderObject *)&g_Cars[g_CameraCarIndex]);
    DrawCars();
    UpdateEnvironment();
    DrawSkyBackground();
    g_RenderState.envMode4 = g_IsEnvironmentMode4;
    DrawTerrainCellsWide();
    DrawCourseObjects();
    DrawCourseScenery2(g_AnimTimer, 1);
}
