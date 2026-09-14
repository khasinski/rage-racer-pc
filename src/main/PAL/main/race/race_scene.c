#include <stdio.h>
#include <stdlib.h>

#include "game/audio.h"
#include <psyz/audio.h>
#include "game/car.h"
#include "game/cd.h"
#include "game/input_internal.h"
#include "game/menu.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_hud_internal.h"
#include "game/random.h"
#include "game/render_internal.h"
#include "game/replay_internal.h"
#include "game/save_internal.h"
#include "game/race_internal.h"
#include "game/race_scene_internal.h"
#include "game/screens.h"
#include "game/scene.h"
#include "game/scene_runtime.h"
#include "game/track.h"
#include "game/track_internal.h"
#include "psyq/snd.h"

/* A retirement is not a finish-line event. Keep following the player's car
 * instead of advancing the autonomous finish camera down the track. */
static s32 s_RetireCameraActive;
/* Optional host diagnostic driver; NULL in ordinary gameplay and tests. */
int (*g_DebugPlayerUpdate)(PlayerCarRuntime *);

/* The authored final-stretch encouragement (0x2A) may still be playing when
 * the car finishes. Wait for its shared voices before announcing Finish!
 * (0x2B), without replaying the encouragement at the finish line. */
static s32 s_FinishFollowupCue = -1;
static s32 s_FinishFollowupWaitFrames;

enum {
    FINISH_CUE_SPECIAL_VOICE_GROUP = 4,
    RACE_END_MUSIC_TRACK = 15,
    PAUSE_TOGGLE_DEBOUNCE = 5,
    PAUSE_RESUME_DEBOUNCE = 30,
    PRE_START_SECTOR = -2,
    INITIAL_RACE_TIME = 15000,
    INITIAL_RIVAL_CUE_FLAGS = 0x1FE,
    RACE_FRAME_SYNC_THRESHOLD = 0x180,
    /* Active speech has enough time to be heard before Finish! may reclaim
     * the shared special voices.  Do not leave that next cue stranded when a
     * host audio backend reports those voices active through the scene exit. */
    FINISH_FOLLOWUP_MAX_WAIT_FRAMES = 60,
};

void QueueFinishFollowupCue(s32 cue) {
    s_FinishFollowupCue = cue;
    s_FinishFollowupWaitFrames = 0;
}

static void UpdateFinishFollowupCue(void) {
    s32 cue;
    s32 specialVoicesActive;

    if (s_FinishFollowupCue < 0) {
        return;
    }
    specialVoicesActive = SpuGetKeyStatus(
        g_SpecialVoiceBits[FINISH_CUE_SPECIAL_VOICE_GROUP]) != 0;
    if (specialVoicesActive &&
        s_FinishFollowupWaitFrames < FINISH_FOLLOWUP_MAX_WAIT_FRAMES) {
        s_FinishFollowupWaitFrames++;
        return;
    }
    cue = ReleaseFinishFollowupCue(
        &s_FinishFollowupCue, 0);
    if (cue < 0) {
        return;
    }
    PlaySoundCue(cue);
}

static s32 RaceRetriesRemaining(void) {
    if (g_CourseProgress == NULL ||
        g_CourseProgress->retriesRemaining <= 0) {
        return 0;
    }
    return g_CourseProgress->retriesRemaining;
}

static s32 UpdateRaceEndState(RaceScene *state) {
    RaceEndFrame frame;

    frame = BuildRaceEndFrame(g_RacePhase, g_GrandPrixMode,
                              RaceRetriesRemaining(),
                              state->fadeTimer);
    if (frame.drawPresentation) {
        if (frame.presentation == RACE_END_PRESENTATION_FINAL) {
            DrawRaceEndBanner(frame.fade);
        } else {
            DrawLostRaceCaption(frame.fade);
        }
        DrawFullscreenFadeTile(frame.fade, 0x49);
    }
    if (frame.startMusic) {
        RequestCdTrack(RACE_END_MUSIC_TRACK);
        StartCdAudio();
    }
    if (frame.exitScene >= 0) {
        ExitRaceScene(frame.exitScene);
        return 1;
    }
    if (!frame.advanceTimer) {
        return 0;
    }
    g_RenderState.mirror.enabled = 0;
    state->fadeTimer = NextRaceFadeTimer(state->fadeTimer);
    return 0;
}

/* Returns non-zero when the selected action has already replaced this scene. */
static s32 UpdateRacePause(RaceScene *state) {
    RacePauseToggleResult toggle;

    toggle = DecideRacePauseToggle(
        g_RacePhase, g_RacePaused, (g_PadPressed & PAD_START) != 0,
        state->pauseDelay, g_GrandPrixMode, state->optionCursor);
    if (!toggle.toggled) {
        return 0;
    }

    state->pauseDelay = PAUSE_TOGGLE_DEBOUNCE;
    g_RacePaused = toggle.paused;
    if (toggle.action == RACE_PAUSE_TOGGLE_RENDERER) {
        PortToggleRenderer();
        PlaySoundCue(2);
        return 0;
    }
    if (toggle.paused) {
        ResetRaceOptionMenuAnimation();
        PauseCdAudio();
        ForceAllEffectVoicesEnabled(0);
        state->optionCursor = 0;
        PlaySoundCue(2);
        return 0;
    }

    if (toggle.action == RACE_PAUSE_QUIT) {
        state->fadeTimer = 0;
        g_RacePhase = RACE_PHASE_QUIT;
        if (g_GrandPrixMode == 0) {
            s32 series = RaceSeriesIndex(g_RaceSeries);
            s32 course = SeriesCourseIndex();

            g_BestLapTimes[series][course][0] =
                g_RankingRecords[series][course][0].raceTime;
        }
        SeedFinishCamera(&g_FinishCamera, &g_PlayerCar);
        StartCdVolumeFade(8);
    } else if (toggle.action == RACE_PAUSE_RETIRE) {
        state->fadeTimer = 0;
        g_RacePhase = RACE_PHASE_RETIRED;
        s_RetireCameraActive = 1;
        if (RaceRetriesRemaining() > 0) {
            PlaySoundCue(0x3D);
        }
        StartCdVolumeFade(8);
    } else if (toggle.action == RACE_PAUSE_RESTART) {
        ExitRaceScene(0xB);
        g_RacePhase = RACE_PHASE_RESTART;
        return 1;
    } else {
        state->pauseDelay = PAUSE_RESUME_DEBOUNCE;
        ForceAllEffectVoicesEnabled(1);
        if (g_RacePhase >= RACE_PHASE_ACTIVE) {
            ResumeCdAudio();
        }
    }
    return 0;
}

int RetireCameraActive(void) {
    return s_RetireCameraActive;
}

void EnterRaceScene(void) {
    RaceScene *state = SceneRuntimeRace();
    s32 course;
    s32 series;
    s32 recordMode;
    s32 i;

    /* Never let the frontend stream overlap the race CD track if a menu
     * transition completes before its visual fade has consumed every step. */
    Psyz_PcmMusicStop();
    SetupDisplay240(0, 0, 0);
    InitRenderState(5);
    ResetReplayWriteCursor();
    ApplyTrackTextureSectionRange();
    InitTrackLighting();
    g_LapCount = CourseLapCount(g_CourseIndex);
    InitPlayerCar(&g_PlayerCar);
    SetTrackTexturePageNow(g_PlayerCar.trackSection);
    BuildStartingGrid();
    course = SeriesCourseIndex();
    series = RaceSeriesIndex(g_RaceSeries);
    g_RaceSeries = series;
    recordMode = RaceRecordMode(g_GrandPrixMode);
    g_LapTimeMs = 0;
    BuildRaceSectorEnds(g_TrackLength, g_SectorEndDistance);
    g_RefSectorTimes.fields.first = g_BestSectorTimes[series][course][0];
    g_RefSectorTimes.fields.second = g_BestSectorTimes[series][course][1];
    g_RefSectorTimes.fields.third = g_BestSectorTimes[series][course][2];
    g_SectorIndex = PRE_START_SECTOR;
    g_SplitSector = 0;
    g_SplitTimer = SPLIT_DISPLAY_FRAMES;
    g_SplitSign = 0;
    g_SplitTargetTime = g_RefSectorTimes.fields.first;
    g_LastSectorTime = -1;
    /* The retail expression builds a 32-bit address through integer/union
     * arithmetic. On a 64-bit host that truncates the native table pointer.
     * This is the same game lookup expressed with its actual dimensions. */
    g_RefLapTime = g_BestLapTimes[series][course][recordMode];
    state->timeRemaining = INITIAL_RACE_TIME;
    g_BestLapThisRace = g_RefLapTime;
    for (i = 0; i < g_LapCount; i++) {
        g_PlayerCar.lapTimes.table.frameCounts[i] = 0;
        g_PlayerCar.lapTimes.table.milliseconds[i] = 0;
    }
    g_RaceTotalTime = 0;
    ResetMirrorState();
    SeekEnvironmentScript(g_TrackRenderTable->environmentScriptOffset);
    BuildTileStrips();
    BuildRaceHudPrims(g_GrandPrixMode);
    g_AnimTimer = 0;
    g_SceneTimer = 0;
    g_Camera.mode = CAMERA_VIEW_CAR;
    g_RacePhase = RACE_PHASE_INTRO;
    s_RetireCameraActive = 0;
    s_FinishFollowupCue = -1;
    s_FinishFollowupWaitFrames = 0;
    g_RaceCueFlags = 0;
    g_RivalCueFlags = INITIAL_RIVAL_CUE_FLAGS;
    for (i = 0; i < RIVAL_CONTENDER_COUNT; i++) {
        g_RivalCueCooldowns[i] = 0;
    }
    InitShuttleScenery();
    SeedFlybyScenery();
    SeedRouteScenery();
    InitPathScenery();
    RequestCdTrack(BgmCdTrack(g_BgmTrack));
    state->pauseDelay = 0;
    state->fadeTimer = 0;
    InitEffectVoiceRuntime();
    g_RivalCueEnabled = 1;
    g_PlayerAutoSteer = 0;
    g_RaceCueDelay = 0;
    g_SceneId = GAME_SCENE_RACE;
    g_FrameSyncThreshold = RACE_FRAME_SYNC_THRESHOLD;
    DrawRoundScreen();
    printf("game0 ok\n");
}

/* The track, its objects, and the scenery, then the same scenery again
 * for the rear-view mirror. The scenery only animates while the race runs;
 * the mirror pass never animates it. */
static void DrawRaceWorld(s32 animateScenery) {
    g_RenderState.geometry.envMode4 = g_IsEnvironmentMode4;
    PortProfileFramePhase("scene_terrain");
    DrawTerrainCells(&g_Camera.view);
    PortProfileFramePhase("scene_course_objects");
    DrawCourseObjects();
    PortProfileFramePhase("scene_scripted_scenery");
    if (g_GrandPrixMode != 0) {
        if (g_GrandPrixClass != GRAND_PRIX_FINAL_CLASS_INDEX) {
            DrawStartGridScenery(g_SceneTimer);
        }
        SetLightMatrix(&g_SceneLightMatrix);
        DrawScriptedScenery(animateScenery);
        PortProfileFramePhase("scene_mirror");
        DrawRearViewMirror(&g_Camera.view, g_SceneTimer);
    }
    PortProfileFramePhase("scene_course_scenery");
    DrawCourseScenery(SeriesCourseIndex(), g_SceneTimer, animateScenery);
    PortProfileFramePhase("scene_mirror_scenery");
    if (BeginMirrorPass() != 0) {
        DrawCourseScenery(SeriesCourseIndex(), g_SceneTimer, 0);
        EndMirrorPass();
    }
    PortProfileFramePhase("scene");
}

static void UpdatePausedRaceScene(RaceScene *state) {
    RacePauseCursorResult cursor;
    s32 move;

    SetReverbDepth(0x28, 0x28);
    cursor = MoveRacePauseCursor(
        g_PadPressed, state->optionCursor, g_GrandPrixMode);
    state->optionCursor = cursor.cursor;
    for (move = 0; move < cursor.moveCount; move++) {
        PlaySoundCue(1);
    }

    DrawRaceOptionMenu(state->optionCursor);
    if (g_GrandPrixMode == 0) {
        DrawSplitTimes();
    }
    DrawRaceHudLabels(g_GrandPrixMode);
    if (g_GrandPrixMode != 0) {
        DrawTimeRemaining(state->timeRemaining);
        DrawRacePosition();
    }
    DrawLapTimes();
    DrawStartCountdown(g_SceneTimer);
    GetTrackZoneBlend(g_PlayerCar.trackProgress);
    DrawPlayerTachometer();

    if ((g_PadHeld &
         RaceCameraButtonMask(g_PadType, g_PadButtonMapping)) &&
        g_Camera.mode == CAMERA_VIEW_CAR &&
        g_RacePhase == RACE_PHASE_ACTIVE) {
        if (g_PadPressed & PAD_R1) {
            g_RenderState.mirror.enabled = 1;
        } else if (g_PadPressed & PAD_L1) {
            g_RenderState.mirror.enabled = 0;
        }
    }

    UpdateCamera(&g_Camera, g_Camera.mode,
                 AsRivalCar(&g_PlayerCar));
    RequestTrackTexturePage(g_PlayerCar.trackSection);
    PortProfileFramePhase("scene_cars");
    if (g_GrandPrixMode != 0) {
        DrawCars();
    }
    PortProfileFramePhase("scene");
    if ((g_PlayerCar.facingBackwards != g_RaceSeries) &&
        WrongWayWarningVisible(g_WrongWayTimer)) {
        DrawWrongWayWarning();
    }
    PortProfileFramePhase("scene_sky");
    DrawSkyBackground(&g_Camera.view);
    PortProfileFramePhase("scene");
    DrawRaceWorld(0);
}

static void UpdateActiveRaceScene(RaceScene *state) {
    s32 lapUpdateResult;
    s32 textureSection;
    RaceClockUpdate raceClock;
    RaceStartUpdate raceStart;
    RaceViewSelection raceView;
    WrongWayUpdate wrongWay;

    lapUpdateResult = 0;
    g_AnimTimer = NextRaceAnimationTimer(g_AnimTimer);
    raceClock = UpdateRaceClock(state->timeRemaining, g_RacePhase,
                                g_GrandPrixMode);
    state->timeRemaining = raceClock.remaining;

    raceStart = UpdateRaceStartState(g_RacePhase, g_SceneTimer);
    g_RacePhase = raceStart.phase;
    if (raceStart.action == RACE_START_ACTION_UPDATE_INTRO_CAMERA) {
        RunRaceIntroCamera(&g_Camera, &g_PlayerCar, g_SceneTimer);
    } else if (raceStart.action == RACE_START_ACTION_BEGIN) {
        BeginCarStandingStart(&g_PlayerCar);
        StartCdAudio();
        state->pauseDelay = 0x1E;
    }

    if (g_RacePhase < RACE_PHASE_FINISHED) {
        DrawStartCountdown(g_SceneTimer);
        PlayCountdownCues(g_SceneTimer);
    }

    if (g_RacePhase < RACE_PHASE_RETIRED) {
        lapUpdateResult = UpdateLapAndFinish(state, &g_PlayerCar, g_GrandPrixMode);
        UpdateSplitTimes(&g_PlayerCar, g_GrandPrixMode, lapUpdateResult);
        if (g_GrandPrixMode == 0 && lapUpdateResult != 2) {
            DrawSplitTimes();
        }
        if (lapUpdateResult < 2) {
            DrawLapTimes();
        }
    }

    if (g_RacePhase < RACE_PHASE_FINISHED) {
        if (g_GrandPrixMode != 0) {
            DrawTimeRemaining(state->timeRemaining);
        }
        if (raceClock.expired) {
            if (RaceRetriesRemaining() > 0) {
                PlaySoundCue(0x3D);
            }
            ForceAllEffectVoicesEnabled(0);
            g_RacePhase = RACE_PHASE_RETIRED;
            state->fadeTimer = 0;
            SeedFinishCamera(&g_FinishCamera, &g_PlayerCar);
            StartCdVolumeFade(8);
        }
    }

    if (g_GrandPrixMode != 0) {
        if (g_RacePhase < RACE_PHASE_FINISHED) {
            UpdateRacePosition();
            DrawRacePosition();
        }
    }
    if (lapUpdateResult < 2 && g_RacePhase < RACE_PHASE_RETIRED) {
        DrawRaceHudLabels(g_GrandPrixMode);
    }

    if (g_RacePhase > RACE_PHASE_INTRO) {
        if (!g_DebugPlayerUpdate || !g_DebugPlayerUpdate(&g_PlayerCar))
            UpdatePlayerCar(&g_PlayerCar);
    } else if (g_RacePhase == RACE_PHASE_INTRO) {
        UpdateLoadedAudioVoices(0, 0);
    }

    if ((g_RacePhase >= RACE_PHASE_ACTIVE) && (g_GrandPrixMode != 0)) {
        UpdateRaceCars();
    }

    if ((g_PadPressed &
         RaceCameraButtonMask(g_PadType, g_PadButtonMapping)) &&
        CanToggleRaceCamera(g_RacePhase)) {
        g_Camera.mode ^= 1;
    }

    raceView = SelectRaceView(g_RacePhase, s_RetireCameraActive,
                              g_Camera.mode);
    g_Camera.mode = raceView.cameraView;
    if (raceView.cameraAction == RACE_CAMERA_ACTION_FINISH) {
        UpdateFinishCamera(&g_Camera, &g_FinishCamera, &g_PlayerCar);
    } else if (raceView.cameraAction == RACE_CAMERA_ACTION_FOLLOW_PLAYER) {
        GameCarRuntime *player =
            AsRivalCar(&g_PlayerCar);
        if (RaceLookBehindActive(g_PadHeld, g_RacePhase,
                                 raceView.cameraView)) {
            UpdateLookBehindCamera(&g_Camera, player);
        } else {
            UpdateCamera(&g_Camera, raceView.cameraView, player);
        }
    }

    textureSection = raceView.useFinishTextureSection
                         ? g_FinishCamera.section
                         : g_PlayerCar.trackSection;
    RequestTrackTexturePage(textureSection);

    PortProfileFramePhase("scene_cars");
    if (g_GrandPrixMode != 0) {
        DrawCars();
    }
    PortProfileFramePhase("scene_environment");
    UpdateEnvironment();
    PortProfileFramePhase("scene_sky");
    DrawSkyBackground(&g_Camera.view);
    PortProfileFramePhase("scene");

    wrongWay = UpdateWrongWayState(
        g_WrongWayTimer,
        g_PlayerCar.facingBackwards != g_RaceSeries, g_RacePhase,
        g_SceneTimer);
    g_WrongWayTimer = wrongWay.timer;
    if (wrongWay.drawWarning) {
        DrawWrongWayWarning();
    }
    if (wrongWay.playCue) {
        PlaySoundCue(0x2C);
    }

    DrawRaceWorld(1);

    GetTrackZoneBlend(g_PlayerCar.trackProgress);
    if (g_RacePhase >= RACE_PHASE_FINISHED) {
        g_ReverbZoneDepth = 0;
    }
    SetReverbDepth(g_ReverbZoneDepth, g_ReverbZoneDepth);
    if ((g_RacePhase != RACE_PHASE_INTRO) && (lapUpdateResult < 2) &&
        (g_RacePhase < RACE_PHASE_RETIRED)) {
        DrawPlayerTachometer();
    }

    if (g_RacePhase < RACE_PHASE_FINISHED) {
        UpdateZoneAmbience(g_PlayerCar.trackProgress);
        UpdatePointAmbience(&g_Camera.view, g_PlayerCar.trackProgress);
        UpdateTrackEventSound(&g_Camera.view, g_PlayerCar.trackSection);
        TriggerRaceCues();
    } else {
        SetPanVoiceTargetVolume(0, 0);
        SetStereoSoundCue(2, 0, 0);
        SetStereoSoundCue(3, 0, 0);
        SetStereoSoundCue(0, 0, 0);
        SetStereoSoundCue(1, 0, 0);
    }
    RecordReplayFrame();
}

void UpdateRaceScene(void) {
    RaceScene *state = SceneRuntimeRace();
    s32 frameStartTimer = NormalizeRaceSceneTimer(g_SceneTimer);

    g_SceneTimer = NextRaceSceneTimer(frameStartTimer);
    UpdateFinishFollowupCue();
    if (g_SceneTimer < 0x3D) {
        DrawRoundScreen();
        DrawFullscreenFadeTile(0xFF - ((g_SceneTimer - 6) * 0xB), 0x49);
    }

    if (state->pauseDelay > 0) {
        state->pauseDelay--;
    }

    if (UpdateRacePause(state)) {
        return;
    }
    if (UpdateRaceEndState(state)) {
        return;
    }

    if (g_RacePaused != 0) {
        g_SceneTimer = frameStartTimer;
        UpdatePausedRaceScene(state);
    } else {
        UpdateActiveRaceScene(state);
    }
}
