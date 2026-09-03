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

#include <limits.h>

enum {
    PROLOGUE_FRAME_SYNC_THRESHOLD = 0x80,
    PROLOGUE_INITIAL_FADE_LEVEL = 0x108,
    PROLOGUE_FADE_IN_STEP = -4,
    PROLOGUE_FADE_OUT_STEP = 4,
    PROLOGUE_FADE_TPAGE = 0x49,
    PROLOGUE_SCENE_ID = 0x20,
    PROLOGUE_DISPLAY_ENABLE_FRAME = 2,
    PROLOGUE_TRACK_SCENE_ID = 6,
    PROLOGUE_SKIP_ENABLE_FRAME = 0x79,
    PROLOGUE_TEXT_FADE_START_FRAME = 0x3C,
    PROLOGUE_TEXT_FADE_END_FRAME = 0x42E,
};

void EnterPrologue(void) {
    SetDispMask(0);
    SetupDisplay240(0, 0, 0);

    g_FrameSyncThreshold = PROLOGUE_FRAME_SYNC_THRESHOLD;
    g_FadeLevel = PROLOGUE_INITIAL_FADE_LEVEL;
    g_FadeStep = PROLOGUE_FADE_IN_STEP;
    g_SceneId = PROLOGUE_SCENE_ID;
    g_PrologueStep = PROLOGUE_STEP_LOAD_TEXTURES;
    g_PrologueCutIndex = 0;
    g_SceneTimer = 0;
    g_CameraCarIndex = 3;
}

static void UpdatePrologueLoad(void) {
    if (g_SceneTimer == PROLOGUE_DISPLAY_ENABLE_FRAME) {
        SetDispMask(1);
    }

    if (g_FadeStep < 0) {
        g_FadeLevel = AdvancePrologueFade(
            g_FadeLevel, g_FadeStep, INT_MAX);

        if (g_FadeLevel == 0) {
            g_FadeStep = 0;
        }

        DrawFullscreenFadeTile(g_FadeLevel, PROLOGUE_FADE_TPAGE);
    } else if (g_FadeStep > 0) {
        g_FadeLevel = AdvancePrologueFade(g_FadeLevel, g_FadeStep, 0x101);

        DrawFullscreenFadeTile(g_FadeLevel, PROLOGUE_FADE_TPAGE);

        if (g_FadeLevel >= 0x101) {
            SetDispMask(0);
            g_CourseIndex = 0;
            InitTrackScene();
            StartCdAudio();
            g_PrologueStep = PROLOGUE_STEP_ACTIVE;
            g_FadeLevel = 0x100;
            g_FadeStep = 0;
        }
    }

    DrawProportionalText(0x5E, 0x72, g_TextNowLoading, 0x7812);
}

static void UpdatePrologueTextureLoad(void) {
    size_t texturePackSize;

    if (AssetLoadCompletedSuccessfully()) {
        if (!AssetSpanSize(g_AssetBase, g_ImageBlockBuffer,
                           &texturePackSize) ||
            !InstallTrackTextureAssetPack(g_AssetBase, texturePackSize)) {
            FailAssetLoad();
        } else {
            RequestTrackDataAssets();
            g_PrologueStep = PROLOGUE_STEP_LOAD_TRACK;
        }
    }

    UpdatePrologueLoad();
}

static void UpdatePrologueTrackLoad(void) {
    if (AssetLoadCompletedSuccessfully()) {
        g_FadeStep = PROLOGUE_FADE_OUT_STEP;
        RequestCdTrack(2);
        g_PrologueStep = PROLOGUE_STEP_WAIT_FOR_FADE;
    }

    UpdatePrologueLoad();
}

static void DrawPrologueText(void) {
    s32 i;
    s32 lineCount = g_PrologueLineCount;
    const s32 scrollY = g_SceneTimer / 3 - 0xD0;
    GameOrderingTableEntry *ot;
    s32 green;
    s32 blue;
    u8 *next;

    if (lineCount < 0) {
        lineCount = 0;
    } else if (lineCount > PROLOGUE_LINE_CAPACITY) {
        lineCount = PROLOGUE_LINE_CAPACITY;
    }
    for (i = 0; i < lineCount; i++) {
        const PrologueLine *line = &g_PrologueLines[i];
        const s32 screenY = line->y - scrollY;
        const s32 intensity = PrologueLineIntensity(screenY);

        if (intensity != 0 && line->text != NULL) {
            GameDrawText8x8Shaded(line->x, screenY, line->text, 0x78CC,
                                  intensity);
        }
    }

    ot = GamePrimaryOrderingTable(1);
    green = g_FadeLevel * 7 / 8 + 0x20;
    blue = g_FadeLevel * 3 / 4 + 0x40;
    next = GameQueueTileTrans(ot, RENDER_PRIM_CURSOR_AS(u8), 0, 0, 0x140,
                              0xF0, g_FadeLevel, green, blue);
    g_RenderState.packetCursor =
        QueueDrawModePrim(ot, next, PROLOGUE_FADE_TPAGE);
}

static void ExitPrologue(void) {
    g_SceneId = PROLOGUE_TRACK_SCENE_ID;
    PauseCdAudio();
    RequestSelectBgmAssets();
}

static void UpdatePrologue(void) {
    s32 timer;
    s32 worldActive;
    s32 eventIndex;

    if (g_SceneTimer == PROLOGUE_DISPLAY_ENABLE_FRAME) {
        SetDispMask(1);
    }

    if (g_SceneTimer >= PROLOGUE_SKIP_ENABLE_FRAME &&
        (g_PadPressed & PAD_CONFIRM)) {
        ExitPrologue();
        return;
    }

    timer = g_SceneTimer;
    if (timer == PROLOGUE_TEXT_FADE_START_FRAME) {
        g_FadeStep = PROLOGUE_FADE_IN_STEP;
    } else if (timer == PROLOGUE_TEXT_FADE_END_FRAME) {
        g_FadeStep = 2;
    } else if (timer == PROLOGUE_END_FRAME) {
        ExitPrologue();
        return;
    }

    g_FadeLevel = AdvancePrologueFade(g_FadeLevel, g_FadeStep, 0xFF);
    if (g_FadeLevel == 0 && g_FadeStep < 0) {
        g_FadeStep = 0;
    } else if (g_FadeLevel == 0xFF && g_FadeStep > 0) {
        g_FadeStep = 0;
    }

    DrawPrologueText();

    worldActive = IsPrologueWorldActive(g_SceneTimer);
    if (worldActive) {
        eventIndex = PrologueCameraCutIndex(g_PrologueCutIndex);
        g_PrologueCutIndex = eventIndex;
        g_AnimTimer = (s32)((u32)g_AnimTimer + 1u);
        if (g_PrologueCameraCuts[eventIndex].timer == g_SceneTimer) {
            if (eventIndex + 1 < PROLOGUE_CAMERA_CUT_COUNT) {
                g_PrologueCutIndex = eventIndex + 1;
            }
            g_CameraCarIndex = PrologueCameraIndex(
                g_PrologueCameraCuts[eventIndex].carIndex);
        }

        g_CameraCarIndex = PrologueCameraIndex(g_CameraCarIndex);

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
    g_SceneTimer = NextPrologueTimer(g_SceneTimer);

    switch (g_PrologueStep) {
    case PROLOGUE_STEP_LOAD_TEXTURES:
        UpdatePrologueTextureLoad();
        break;
    case PROLOGUE_STEP_LOAD_TRACK:
        UpdatePrologueTrackLoad();
        break;
    case PROLOGUE_STEP_WAIT_FOR_FADE:
        UpdatePrologueLoad();
        break;
    case PROLOGUE_STEP_ACTIVE:
        UpdatePrologue();
        break;
    }
}
