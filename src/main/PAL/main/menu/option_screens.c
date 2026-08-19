#include "common.h"
#include "game/game_input.h"
#include "game/prim.h"
#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/menu_controller.h"
#include "game/menu_dialog_controller.h"
#include "game/menu_runtime.h"
#include "game/option_controller.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/render_workspace.h"
#include "game/state.h"
#include "game/track.h"
#include "game/game_context.h"
#include "psyq/gpu.h"


/* g_GameModeHandlers[5]: left/right edits the selected audio setting, cancel restores it. */
void UpdateSoundSettingAdjust(void) {
    s32 *setting = NULL;
    s32 maximum = 15;
    MenuDialogInputResult input;

    DrawSoundOptionScreen();

    switch (g_SoundOptionCursor) {
    case 0:
        setting = &g_BgmVolumeSetting;
        break;
    case 1:
        setting = &g_SfxVolumeSetting;
        break;
    case 2:
        setting = &g_MonoOutput;
        maximum = 1;
        break;
    }

    if (setting != NULL) {
        input = MenuDialogHandleRange(
            *setting, 0, maximum, -1, 0,
            g_GameInput.pressed, g_GameInput.pressed);
        *setting = input.value;
        if (input.moveCount != 0) PlaySoundCue(1);
        if (input.confirmed) {
            g_GameMode = 4;
        } else if (input.cancelled) {
            g_GameMode = 4;
            *setting = g_ScreenOffsetEditX;
        }
    }
    ApplyAudioSettings();
    if (MenuResolveAction(g_GameInput.pressed, PAD_CONFIRM, PAD_CANCEL) ==
        MENU_ACTION_CONFIRM) {
        PlaySoundCue(2);
    } else if (MenuResolveAction(
                   g_GameInput.pressed, PAD_CONFIRM, PAD_CANCEL) ==
               MENU_ACTION_CANCEL) {
        PlaySoundCue(3);
    }
}

void DrawScreenAdjustScreen(void) {
    OT_TYPE *base = GamePrimaryOrderingTable(51);
    s32 color = 0x7F40;
    s32 y48 = 0x48;
    s32 h18 = 0x18;
    s32 w0c = 0xC;
    u8 **scratch = &RENDER_PRIM_CURSOR_AS(u8);
    u8 *next;

    next = *scratch;
    next = GameQueueSpriteTrans(base, next, 0x9A, 0x88, w0c, h18, 0xC8, y48, color);
    next = GameQueueSpriteTrans(base, next, 0x9A, 0xB8, w0c, h18, 0xD4, y48, color);
    next = GameQueueSpriteTrans(base, next, 0xA6, 0xA0, w0c, h18, 0xE0, y48, color);
    *scratch = GameQueueSpriteTrans(base, next, 0x8E, 0xA0, w0c, h18, 0xEC, y48, color);
    DrawOptionHintBar(3);
}

/* g_GameModeHandlers[6]: moves the screen offset and commits it to g_ScreenOffsetX/Y. */
void UpdateScreenAdjustScreen(void) {
    ScreenAdjustState state;
    ScreenAdjustResult result;

    DrawScreenAdjustScreen();

    state = (ScreenAdjustState){g_ScreenOffsetEditX, g_ScreenOffsetEditY};
    result = ScreenAdjustReduce(
        &state, g_GameInput.pressed, g_GameInput.pressedRepeat);
    g_ScreenOffsetEditX = state.x;
    g_ScreenOffsetEditY = state.y;
    if (result.moved) MenuFlowApplyEffects(MENU_RUNTIME_EFFECT_MOVE);
    if (result.decision == OPTION_DECISION_ACCEPT) {
        MenuFlowApplyEffects(MENU_RUNTIME_EFFECT_ACCEPT);
        g_GameMode = 1;
        g_ScreenOffsetX.value = g_ScreenOffsetEditX;
        g_ScreenOffsetY.value = g_ScreenOffsetEditY;
    } else if (result.decision == OPTION_DECISION_CANCEL) {
        MenuFlowApplyEffects(MENU_RUNTIME_EFFECT_BACK);
        g_GameMode = 1;
        g_ScreenOffsetEditX = g_ScreenOffsetX.value;
        g_ScreenOffsetEditY = g_ScreenOffsetY.value;
    }

    g_DispEnv0ScreenX = g_ScreenOffsetEditX;
    g_DispEnv0ScreenY = g_ScreenOffsetEditY + 29;
    g_DispEnv1ScreenX = g_ScreenOffsetEditX;
    g_DispEnv1ScreenY = g_ScreenOffsetEditY + 29;
}

void DrawOptionSceneOverlay(void) {
    u8 **scratch;
    void *base;
    u8 *pkt;
    s32 target;
    s32 value;
    s32 w120;
    s32 two;
    s32 white;
    s32 h1c0;

    if (g_GameMode != 9) {
        DrawPadTypeHint();
    }

    target = 0xF0;
    if (g_GameMode == 6) {
        target = 0x1E0;
    }

    value = g_OptionLetterboxHeight;
    if (value < target) {
        g_OptionLetterboxHeight = value + 4;
    } else if (target < value) {
        g_OptionLetterboxHeight = value - 4;
    }

    scratch = &RENDER_PRIM_CURSOR_AS(u8);
    /* The OPTION backdrop lives behind the depth-0/51 2D foreground. */
    base = GamePrimaryOrderingTable(54);
    pkt = *scratch;

    if (g_GameMode == 6) {
        w120 = 0x120;
        two = 2;
        white = 0xFF;
        pkt = AddTilePrim(base, pkt, 0x10, 0x20, w120, two, white, white, white);
        pkt = AddTilePrim(base, pkt, 0x10, 0x1C0, w120, two, white, white, white);
        h1c0 = 0x1C0;
        pkt = GameQueueLine(base, pkt, 0x10, 0x20, 0x10, h1c0, white, white, white);
        pkt = GameQueueLine(base, pkt, 0x130, 0x20, 0x130, h1c0, white, white, white);
    }

    *scratch = AddTilePrim(base, pkt, 0, 0, 0x140, g_OptionLetterboxHeight, 0x85, 0x15, 0xE);
}

/* Scene 23: the setup / OPTION scene, dispatching g_GameModeHandlers[g_GameMode]. */
void UpdateOptionScene(void) {
    RENDER_PRIM_CURSOR_AS(u8) = AddTilePrim(
        GamePrimaryOrderingTable(0), RENDER_PRIM_CURSOR_AS(u8), 0, 0, 0x140, 2, 0, 0, 0);
    g_AnimTimer = g_AnimTimer + 1;
    g_SceneTimer = g_SceneTimer + 1;
    if (g_SceneTimer == 2) {
        SetDispMask(1);
    }
    g_GameModeHandlers[g_GameMode]();
    DrawOptionSceneOverlay();
}

void InitTrackScene(void) {
    InitRenderState(5);
    LoadTrackTexturePageRange();
    InitTrackLighting();
    g_TrackWalkStart = g_TrackEventData->trackWalkStart;
    BuildStartingGrid();
    SetTrackTexturePageNow(g_Cars[g_CameraCarIndex].trackSection);
    SeekEnvironmentScript(g_TrackRenderTable->environmentScriptOffset);
    g_CameraViewMode = CAMERA_VIEW_TRACK;
    g_AnimTimer = 0;
    g_SceneTimer = 0;
    g_FrameSyncThreshold = 0x180;
    InitShuttleScenery();
}

void EnterBgmSelectScreen(void) {
    SetDispMask(0);
    SetupDisplay240(0, 0, 0);
    g_FrameSyncThreshold = 0x80;
    g_FadeLevel = 0x13C;
    g_FadeStep = -4;
    GameSceneSet(SCENE_BGM_SELECT);
    g_BgmSelectCursor = 1;
    g_BgmSelectShowUi = 1;
    g_BgmSelectCdTrack = 3;
    g_BgmSelectStep = BGM_SELECT_STEP_LOAD_ASSETS;
    g_SceneTimer = 0;
    g_BgmSelectTrack = 0;
    g_BgmChangeDelay = 0x1E;
    g_CdTrackEnded = 0;
    g_CameraCarIndex = 0;
}

void UpdateOptionSceneFade(void) {
    s32 d;
    s32 v;
    if (g_SceneTimer == 0xF) {
        SetDispMask(1);
    }
    d = g_FadeStep;
    if (d < 0) {
        s32 e = g_FadeLevel;
        e += d;
        g_FadeLevel = e;
        if (g_FadeLevel < 0) {
            g_FadeLevel = 0;
            g_FadeStep = 0;
        }
        DrawFullscreenFadeTile(g_FadeLevel, 0x49);
    } else if (d > 0) {
        s32 e = g_FadeLevel;
        e += d;
        v = e;
        g_FadeLevel = v;
        DrawFullscreenFadeTile(v, 0x49);
        if (g_FadeLevel >= 257) {
            SetDispMask(0);
            InitTrackScene();
            g_FadeStep = 0;
            g_FadeLevel = 0;
            g_BgmSelectStep = BGM_SELECT_STEP_ACTIVE;
        }
    }
    DrawProportionalText(0x5E, 0x72, g_TextNowLoading, 0x7812);
}

void UpdateBgmSelectLoad(void) {
    if (g_AssetLoadState == 0) {
        InstallCourseAssets();
        RequestTrackDataAssets();
        g_BgmSelectStep = BGM_SELECT_STEP_FADE_IN;
    }

    UpdateOptionSceneFade();
}

void UpdateBgmSelectFadeIn(void) {
    if (g_AssetLoadState == 0) {
        g_FadeStep = 4;
    }
    UpdateOptionSceneFade();
}
