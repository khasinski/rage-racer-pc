#include "game/prim.h"
#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/fmv_internal.h"
#include "game/car.h"
#include "game/cd.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/race_internal.h"
#include "game/state.h"
#include "game/track.h"

void UpdateAttractDemoScene(void) {
    g_AttractDemoSteps[g_AttractDemoStep]();

    if ((g_SceneId == 0x1E) && ((g_PadPressed & PAD_CONFIRM) != 0)) {
        if (g_AssetLoadState != 0) {
            ResetAssetLoader();
            g_SceneId = 3;
            g_StreamReturnScene = 0;
        } else {
            ReturnToTitleScene();
        }
    }
}

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

void UpdatePrologueLoad(void) {
    s32 delta;

    if (g_SceneTimer == 2) {
        SetDispMask(1);
    }

    delta = g_FadeStep;
    if (delta < 0) {
        s32 value;

        value = g_FadeLevel;
        value = value + delta;
        g_FadeLevel = value;

        if (g_FadeLevel < 0) {
            g_FadeLevel = 0;
            g_FadeStep = 0;
        }

        DrawFullscreenFadeTile(g_FadeLevel, 0x49);
    } else if (delta > 0) {
        s32 value;

        value = g_FadeLevel;
        value = value + delta;
        g_FadeLevel = value;

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

void UpdatePrologueLoadStep0(void) {
    if (g_AssetLoadState == 0) {
        InstallCourseAssets();
        RequestTrackDataAssets();
        g_PrologueStep = 1;
    }

    UpdatePrologueLoad();
}

void UpdatePrologueLoadStep1(void) {
    if (g_AssetLoadState == 0) {
        g_FadeStep = 4;
        RequestCdTrack(2);
        g_PrologueStep = 2;
    }

    UpdatePrologueLoad();
}

void UpdatePrologueLoadStep2(void) {
    UpdatePrologueLoad();
}

void DrawPrologueText(void) {
    s32 i;
    s32 adjusted;
    u32 timer;
    s32 delta;
    s32 value;
    s32 clamped;
    u8 *next;
    u8 *prim;
    s32 green;
    s32 blueScale;
    s32 blue;

    for (i = 0; i < g_PrologueLineCount; i++) {
        s32 tableY;

        timer = g_SceneTimer;
        adjusted = (timer / 3) - 0xD0;
        tableY = g_PrologueLines[i].y;
        delta = tableY - adjusted;

        if (delta < 0x60) {
            value = (0x60 - delta) << 1;
        } else if (delta >= 0x91) {
            value = (delta - 0x90) << 1;
        } else {
            value = 0;
        }

        if (value >= 0) {
            clamped = value;
            if (clamped >= 0x80) {
                clamped = 0x7F;
            }
        } else {
            clamped = 0;
        }

        value = 0x7F - clamped;
        if (value != 0) {
            GameDrawText8x8Shaded(
                g_PrologueLines[i].x,
                delta,
                g_PrologueLines[i].text,
                0x78CC,
                value);
        }
    }
    {
        s32 fadeLevel;
        u8 **scratch;
        u8 *ptr;
        s32 greenScale;
        s32 tmp;

        fadeLevel = g_FadeLevel;
        scratch = &SCRATCH_PRIM_CURSOR_AS(u8);
        tmp = fadeLevel * 7;
        greenScale = tmp * 32;
        prim = *scratch;
        ptr = (u8 *)GamePrimaryOrderingTable(1);
        green = (greenScale / 0x100) + 0x20;
        blueScale = (fadeLevel * 3) << 6;
        blue = (blueScale / 0x100) + 0x40;

        next = GameQueueTileTrans(ptr, prim, 0, 0, 0x140, 0xF0, fadeLevel, green, blue);
        *scratch = QueueDrawModePrim(ptr, next, 0x49);
    }
}

void ExitPrologue(void) {
    g_SceneId = 6;
    PauseCdAudio();
    RequestSelectBgmAssets();
}

void UpdatePrologue(void) {
    s32 timer;
    u32 active;
    s32 eventIndex;

    if (g_SceneTimer == 2) {
        SetDispMask(1);
    }

    {
        u32 sceneFrame = g_SceneTimer;
        if (sceneFrame >= 0x79 && (g_PadPressed & PAD_CONFIRM)) {
            ExitPrologue();
        }
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

    active = g_SceneTimer - 0x10;
    active = active < 0x40F;
    if (active) {
        eventIndex = g_PrologueCutIndex;
        g_AnimTimer++;
        if (g_PrologueCameraCuts[eventIndex].timer == g_SceneTimer) {
            g_PrologueCutIndex = eventIndex + 1;
            g_CameraCarIndex = g_PrologueCameraCuts[eventIndex].carIndex;
        }

        UpdateAttractCars();

        RequestTrackTexturePage(g_Cars[g_CameraCarIndex].trackSection);

        UpdateCamera(g_CameraViewMode, (GameRenderObject *)&g_Cars[g_CameraCarIndex]);
        UpdateEnvironment();
    }

    DrawCars();
    DrawSkyBackground();
    SCRATCH_ENV_MODE4 = g_IsEnvironmentMode4;
    DrawTerrainCellsWide();
    DrawCourseObjects();
    DrawCourseScenery2(g_AnimTimer, active);
}
