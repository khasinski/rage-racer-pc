#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/audio_internal.h"
#include "game/fmv_internal.h"
#include "game/car.h"
#include "game/cd.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/scene.h"
#include "game/scene_runtime.h"
#include "game/render_internal.h"
#include "game/track.h"

enum {
    ATTRACT_FRAME_SYNC_THRESHOLD = 0x80,
    ATTRACT_INITIAL_TITLE_FADE = 0x80,
    ATTRACT_DISPLAY_ENABLE_FRAME = 2,
    ATTRACT_TITLE_END_FRAME = 0x3D,
    ATTRACT_EXIT_WASH_START_FRAME = 0x6CD,
    ATTRACT_EXIT_AUDIO_FADE_FRAMES = 0x38,
    ATTRACT_FADE_TPAGE = 0x49,
};

static const s16 s_titleDelays[] = {15, 256};

void EnterAttractDemo(void) {
    AttractDemo *state = SceneRuntimeAttractDemo();
    size_t texturePackSize;

    SetDispMask(0);
    SetupDisplay240(0, 0, 0);

    g_FrameSyncThreshold = ATTRACT_FRAME_SYNC_THRESHOLD;
    if (!UploadImageAsset(GetImageAssetHeaderWords(g_ImageBlockBuffer),
                          g_ImageBlockSize)) {
        FailAssetLoad();
        return;
    }
    if (!AssetSpanSize(g_AssetBase, g_ImageBlockBuffer, &texturePackSize) ||
        !InstallTrackTextureAssetPack(g_AssetBase, texturePackSize)) {
        FailAssetLoad();
        return;
    }
    RequestTrackDataAssets();

    state->step = ATTRACT_DEMO_STEP_LOAD;
    g_FadeLevel = ATTRACT_INITIAL_TITLE_FADE;
    g_SceneTimer = 0;
    g_SceneId = GAME_SCENE_ATTRACT_DEMO;
    g_CameraCarIndex = 0;
}

static s32 GetAttractTitleFade(AttractDemo *state, s32 element) {
    if (state->step != ATTRACT_DEMO_STEP_LOAD && g_FadeLevel > 0) {
        g_FadeLevel--;
    }

    return AttractTitleFadeLevel(state->step, g_SceneTimer, g_FadeLevel,
                                 s_titleDelays[element]);
}

static void DrawAttractTitle(AttractDemo *state) {
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    s32 fade;

    fade = GetAttractTitleFade(state, 0);
    DrawSprite(ot, 0x74, 0x34, 0x58, 0x38, 0xA8, 0xA8, fade, fade, fade,
               0x1F, 0, 1, 0x29);
    DrawSprite(ot, 0x44, 0x70, 0xB8, 0x14, 0x48, 0xE8, fade, fade, fade,
               0x80, 0, 1, 0x29);
    fade = GetAttractTitleFade(state, 1);
    DrawSprite(ot, 0x5E, 0x90, 0x84, 0xC, 0,
               g_CourseIndex * 12 + 0x9C, fade, fade, fade, 0x12, 0, 1,
               0x29);
}

static void UpdateAttractDemoStart(AttractDemo *state) {
    s32 shuffleTrack;
    s32 cdTrack;
    g_SceneTimer = NextAttractLoadTimer(g_SceneTimer);

    if (g_SceneTimer == ATTRACT_DISPLAY_ENABLE_FRAME) {
        SetDispMask(1);
    }

    if (AssetLoadCompletedSuccessfully()) {
        InitTrackScene();

        state->step = ATTRACT_DEMO_STEP_RACE;
        shuffleTrack = BgmShuffleTrackAt(g_BgmShuffleOrder, g_BgmTrackCount,
                                         g_BgmShuffleIndex);
        AdvanceBgmShuffleBag(shuffleTrack);

        cdTrack = BgmCdTrack(shuffleTrack);
        RequestCdTrack(cdTrack);
        StartCdAudio();
    }

    DrawAttractTitle(state);
}

static void ReturnToTitleScene(void) {
    g_SceneId = GAME_SCENE_ENTER_TITLE;
    g_Fmv.returnScene = 0;
    ResetCdAudioState();
}

static void UpdateAttractDemoRace(AttractDemo *state) {
    s32 timer;

    g_SceneTimer = NextAttractRaceTimer(g_SceneTimer);
    timer = g_SceneTimer;
    if (timer < ATTRACT_TITLE_END_FRAME) {
        DrawAttractTitle(state);
        DrawFullscreenFadeTile(AttractOpeningWashLevel(timer),
                               ATTRACT_FADE_TPAGE);
    }

    timer = g_SceneTimer;
    if (ShouldStartAttractExitFade(timer)) {
        StartCdVolumeFade(ATTRACT_EXIT_AUDIO_FADE_FRAMES);
    }
    if (timer >= ATTRACT_EXIT_WASH_START_FRAME) {
        DrawFullscreenFadeTile(AttractClosingWashLevel(timer),
                               ATTRACT_FADE_TPAGE);
    }

    if (ShouldReturnFromAttractDemo(timer)) {
        ReturnToTitleScene();
        return;
    }

    g_AnimTimer = (s32)((u32)g_AnimTimer + 1);
    g_CameraCarIndex = CycleAttractCameraCar(0xFF, g_CameraCarIndex);
    UpdateAndDrawAttractWorld();
}

void UpdateAttractDemoScene(void) {
    AttractDemo *state = SceneRuntimeAttractDemo();

    switch (state->step) {
    case ATTRACT_DEMO_STEP_INVALID:
        break;
    case ATTRACT_DEMO_STEP_LOAD:
        UpdateAttractDemoStart(state);
        break;
    case ATTRACT_DEMO_STEP_RACE:
        UpdateAttractDemoRace(state);
        break;
    }

    if (g_SceneId == GAME_SCENE_ATTRACT_DEMO &&
        (g_PadPressed & PAD_CONFIRM) != 0) {
        if (!AssetLoadCompletedSuccessfully()) {
            ResetAssetLoader();
            g_SceneId = GAME_SCENE_ENTER_TITLE;
            g_Fmv.returnScene = 0;
        } else {
            ReturnToTitleScene();
        }
    }
}
