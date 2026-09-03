#include "game/prim.h"
#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/car.h"
#include "game/cd.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/race_internal.h"
#include "game/state.h"
#include "game/track.h"

void EnterPrologue(void) {
    SetDispMask(0);
    SetupDisplay240(0, 0, 0);

    g_FrameSyncThreshold = 0x80;
    g_FadeLevel = 0x108;
    g_FadeStep = -4;
    g_SceneId = 0x20;
    g_PrologueStep = 0;
    g_PrologueCutIndex = 0;
    g_SceneTimer = 0;
    g_CameraCarIndex = 3;
}

static void UpdatePrologueLoad(void) {
    if (g_SceneTimer == 2) {
        SetDispMask(1);
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
            SetDispMask(0);
            g_CourseIndex = 0;
            InitTrackScene();
            StartCdAudio();
            g_PrologueStep = 3;
            g_FadeLevel = 0x100;
            g_FadeStep = 0;
        }
    }

    DrawProportionalText(0x5E, 0x72, g_TextNowLoading, 0x7812);
}

static void UpdatePrologueLoadStep0(void) {
    if (AssetLoadCompletedSuccessfully()) {
        if (g_ImageBlockBuffer > g_AssetBase &&
            InstallTrackTextureAssetPack(
                g_AssetBase,
                (size_t)(g_ImageBlockBuffer - g_AssetBase))) {
            RequestTrackDataAssets();
            g_PrologueStep = 1;
        }
    }

    UpdatePrologueLoad();
}

static void UpdatePrologueLoadStep1(void) {
    if (AssetLoadCompletedSuccessfully()) {
        g_FadeStep = 4;
        RequestCdTrack(2);
        g_PrologueStep = 2;
    }

    UpdatePrologueLoad();
}

static void UpdatePrologueLoadStep2(void) {
    UpdatePrologueLoad();
}

static void DrawPrologueText(void) {
    s32 i;
    const s32 scrollY = g_SceneTimer / 3 - 0xD0;
    GameOrderingTableEntry *ot;
    s32 green;
    s32 blue;
    u8 *next;

    for (i = 0; i < g_PrologueLineCount; i++) {
        const PrologueLine *line = &g_PrologueLines[i];
        const s32 screenY = line->y - scrollY;
        const s32 intensity = PrologueLineIntensity(screenY);

        if (intensity != 0) {
            GameDrawText8x8Shaded(line->x, screenY, line->text, 0x78CC,
                                  intensity);
        }
    }

    ot = GamePrimaryOrderingTable(1);
    green = g_FadeLevel * 7 / 8 + 0x20;
    blue = g_FadeLevel * 3 / 4 + 0x40;
    next = GameQueueTileTrans(ot, RENDER_PRIM_CURSOR_AS(u8), 0, 0, 0x140,
                              0xF0, g_FadeLevel, green, blue);
    g_RenderState.packetCursor = QueueDrawModePrim(ot, next, 0x49);
}

static void ExitPrologue(void) {
    g_SceneId = 6;
    PauseCdAudio();
    RequestSelectBgmAssets();
}

static void UpdatePrologue(void) {
    s32 timer;
    s32 worldActive;
    s32 eventIndex;

    if (g_SceneTimer == 2) {
        SetDispMask(1);
    }

    if (g_SceneTimer >= 0x79 && (g_PadPressed & PAD_CONFIRM)) {
        ExitPrologue();
    }

    timer = g_SceneTimer;
    if (timer == 0x3C) {
        g_FadeStep = -4;
    } else if (timer == 0x42E) {
        g_FadeStep = 2;
    } else if (timer == 0x500) {
        ExitPrologue();
    }

    g_FadeLevel += g_FadeStep;
    if (g_FadeLevel < 0) {
        g_FadeLevel = 0;
        g_FadeStep = 0;
    } else if (g_FadeLevel >= 0x100) {
        g_FadeLevel = 0xFF;
        g_FadeStep = 0;
    }

    DrawPrologueText();

    worldActive = IsPrologueWorldActive(g_SceneTimer);
    if (worldActive) {
        eventIndex = g_PrologueCutIndex;
        g_AnimTimer++;
        if (g_PrologueCameraCuts[eventIndex].timer == g_SceneTimer) {
            g_PrologueCutIndex = eventIndex + 1;
            g_CameraCarIndex = g_PrologueCameraCuts[eventIndex].carIndex;
        }

        UpdateAttractCars();

        RequestTrackTexturePage(g_Cars[g_CameraCarIndex].trackSection);

        UpdateCamera(g_CameraViewMode,
                     GetCarRenderObject(&g_Cars[g_CameraCarIndex]));
        UpdateEnvironment();
    }

    DrawCars();
    DrawSkyBackground();
    g_RenderState.envMode4 = g_IsEnvironmentMode4;
    DrawTerrainCellsWide();
    DrawCourseObjects();
    DrawPresentationCourseScenery(g_AnimTimer, worldActive);
}

void TickPrologueStep(void) {
    g_SceneTimer++;

    switch (g_PrologueStep) {
    case 0:
        UpdatePrologueLoadStep0();
        break;
    case 1:
        UpdatePrologueLoadStep1();
        break;
    case 2:
        UpdatePrologueLoadStep2();
        break;
    case 3:
        UpdatePrologue();
        break;
    }
}
