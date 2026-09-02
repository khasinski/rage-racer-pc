#include "game/asset.h"
#include "game/audio_internal.h"
#include "game/fmv_internal.h"
#include "game/car.h"
#include "game/cd.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/render_internal.h"
#include "game/track.h"


void EnterAttractDemo(void) {
    SetDispMask(0);
    SetupDisplay240(0, 0, 0);

    g_FrameSyncThreshold = 0x80;
    UploadImageAsset(GetImageAssetHeaderWords(g_ImageBlockBuffer));
    InstallCourseAssets();
    RequestTrackDataAssets();

    g_AttractDemoStep = ATTRACT_DEMO_STEP_LOAD;
    g_FadeLevel = 0x80;
    g_SceneTimer = 0;
    g_SceneId = 0x1E;
    g_CameraCarIndex = 0;
}

static s32 GetAttractTitleFade(s32 element) {
    if (g_AttractDemoStep != ATTRACT_DEMO_STEP_LOAD && g_FadeLevel > 0) {
        g_FadeLevel--;
    }

    return AttractTitleFadeLevel(g_AttractDemoStep, g_SceneTimer, g_FadeLevel,
                                 g_AttractTitleDelays[element]);
}

static void DrawAttractTitle(void) {
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    s32 fade;

    fade = GetAttractTitleFade(0);
    DrawSprite(ot, 0x74, 0x34, 0x58, 0x38, 0xA8, 0xA8, fade, fade, fade,
               0x1F, 0, 1, 0x29);
    DrawSprite(ot, 0x44, 0x70, 0xB8, 0x14, 0x48, 0xE8, fade, fade, fade,
               0x80, 0, 1, 0x29);
    fade = GetAttractTitleFade(1);
    DrawSprite(ot, 0x5E, 0x90, 0x84, 0xC, 0,
               g_CourseIndex * 12 + 0x9C, fade, fade, fade, 0x12, 0, 1,
               0x29);
}

static void UpdateAttractDemoStart(void) {
    s32 shuffleTrack;
    s32 cdTrack;
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
        shuffleTrack = g_BgmShuffleOrder[g_BgmShuffleIndex];
        AdvanceBgmShuffleBag(shuffleTrack);

        cdTrack = BgmCdTrack(shuffleTrack);
        RequestCdTrack(cdTrack);
        StartCdAudio();
    }

    DrawAttractTitle();
}

void ReturnToTitleScene(void) {
    g_SceneId = 3;
    g_StreamReturnScene = 0;
    ResetCdAudioState();
}

static void UpdateAttractDemoRace(void) {
    u32 timer;

    g_SceneTimer++;
    timer = g_SceneTimer;
    if (timer < 0x3D) {
        DrawAttractTitle();
        DrawFullscreenFadeTile(AttractOpeningWashLevel(timer), 0x49);
    }

    timer = g_SceneTimer;
    if (ShouldStartAttractExitFade(timer)) {
        StartCdVolumeFade(0x38);
    }
    if (timer >= 0x6CD) {
        DrawFullscreenFadeTile(AttractClosingWashLevel(timer), 0x49);
    }

    if (ShouldReturnFromAttractDemo(timer)) {
        ReturnToTitleScene();
    }

    g_AnimTimer++;
    g_CameraCarIndex = CycleAttractCameraCar(0xFF, g_CameraCarIndex);
    UpdateAndDrawAttractWorld();
}

void UpdateAttractDemoScene(void) {
    switch (g_AttractDemoStep) {
    case ATTRACT_DEMO_STEP_INVALID:
        break;
    case ATTRACT_DEMO_STEP_LOAD:
        UpdateAttractDemoStart();
        break;
    case ATTRACT_DEMO_STEP_RACE:
        UpdateAttractDemoRace();
        break;
    }

    if (g_SceneId == 0x1E && (g_PadPressed & PAD_CONFIRM) != 0) {
        if (g_AssetLoadState != 0) {
            ResetAssetLoader();
            g_SceneId = 3;
            g_StreamReturnScene = 0;
        } else {
            ReturnToTitleScene();
        }
    }
}
