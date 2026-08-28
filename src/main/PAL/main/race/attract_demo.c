#include "game/asset.h"
#include "game/audio_internal.h"
#include "game/fmv_internal.h"
#include "game/car.h"
#include "game/cd.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/track.h"


void UpdateBgmSelectScene(void) {
    void (*func)(void);

    func = g_BgmSelectSteps[g_BgmSelectStep];
    g_SceneTimer++;
    func();
}

void EnterAttractDemo(void) {
    s32 initialValue;

    SetDispMask(0);
    SetupDisplay240(0, 0, 0);

    initialValue = 0x80;
    g_FrameSyncThreshold = initialValue;
    UploadImageAsset(g_ImageBlockBuffer);
    InstallCourseAssets();
    RequestTrackDataAssets();

    g_AttractDemoStep = ATTRACT_DEMO_STEP_LOAD;
    g_FadeLevel = initialValue;
    g_SceneTimer = 0;
    g_SceneId = 0x1E;
    g_CameraCarIndex = 0;
}

s32 GetAttractTitleFade(s32 element) {
    s32 value;

    if (g_AttractDemoStep == ATTRACT_DEMO_STEP_LOAD) {
        value = (g_SceneTimer * 4) - g_AttractTitleDelays[element];
    } else {
        if (g_FadeLevel > 0) {
            g_FadeLevel--;
        }
        value = g_FadeLevel;
    }

    return value < 0 ? 0 : (value < 0x80 ? value : 0x7F);
}

void DrawAttractTitle(void) {
    u8 *ptr;
    s32 value;
    u32 one;
    u32 flags;

    ptr = (u8 *)GamePrimaryOrderingTable(0);
    value = GetAttractTitleFade(0);
    one = 1;
    flags = 0x29;
    DrawSprite(ptr, 0x74, 0x34, 0x58, 0x38, 0xA8, 0xA8, value, value, value, 0x1F, 0, one, flags);
    DrawSprite(ptr, 0x44, 0x70, 0xB8, 0x14, 0x48, 0xE8, value, value, value, 0x80, 0, one, flags);
    value = GetAttractTitleFade(1);
    DrawSprite(ptr, 0x5E, 0x90, 0x84, 0xC, 0, (g_CourseIndex * 12) + 0x9C, value, value, value, 0x12, 0, one, flags);
}

void UpdateAttractDemoStart(void) {
    s32 mode;
    u32 timer;

    timer = g_SceneTimer;
    if (timer < 0x2710) {
        g_SceneTimer = timer + 1;
    }

    if (g_SceneTimer == 2) {
        SetDispMask(1);
    }

    if (g_AssetLoadState == 0) {
        InitTrackScene();

        g_AttractDemoStep = ATTRACT_DEMO_STEP_RACE;
        mode = g_BgmShuffleOrder[g_BgmShuffleIndex];
        AdvanceBgmShuffleBag(mode);

        mode += 3;
        if (mode == 0xC) {
            mode = 0x11;
        }

        RequestCdTrack(mode);
        StartCdAudio();
    }

    DrawAttractTitle();
}

void ReturnToTitleScene(void) {
    g_SceneId = 3;
    g_StreamReturnScene = 0;
    ResetCdAudioState();
}

void UpdateAttractDemoRace(void) {
    u32 value;
    u32 timer;
    s32 index;

    g_SceneTimer++;
    timer = g_SceneTimer;
    if (timer < 0x3D) {
        DrawAttractTitle();
        value = g_SceneTimer - 6;
        DrawFullscreenFadeTile(0xFF - (((value * 3) * 4) - value), 0x49);
    }

    timer = g_SceneTimer;
    if (timer == 0x6CC) {
        StartCdVolumeFade(0x38);
        timer = g_SceneTimer;
    }
    if (timer >= 0x6CD) {
        u32 adjusted;

        adjusted = timer - 0x6CC;
        DrawFullscreenFadeTile(adjusted * 5, 0x49);
    }

    if (g_SceneTimer == 0x708) {
        ReturnToTitleScene();
    }

    g_AnimTimer++;
    g_CameraCarIndex = CycleAttractCameraCar(0xFF, g_CameraCarIndex);
    UpdateAttractCars();

    index = g_CameraCarIndex;
    RequestTrackTexturePage(g_Cars[index].trackSection);

    UpdateCamera(g_CameraViewMode, (GameRenderObject *)&g_Cars[g_CameraCarIndex]);
    DrawCars();
    UpdateEnvironment();
    DrawSkyBackground();
    SCRATCH_ENV_MODE4 = g_IsEnvironmentMode4;
    DrawTerrainCellsWide();
    DrawCourseObjects();
    DrawCourseScenery2(g_AnimTimer, 1);
}
