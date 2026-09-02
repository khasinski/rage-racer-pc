#include <stdio.h>
#include <stdlib.h>

#include "game/audio.h"
#include "game/cd.h"
#include "game/diagnostics.h"
#include "game/input_internal.h"
#include "game/menu.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/random.h"
#include "game/render_internal.h"
#include "game/save_internal.h"
#include "game/race_internal.h"
#include "game/race_scene_internal.h"
#include "game/screens.h"
#include "game/track.h"
#include "psyq/snd.h"

/* A retirement is not a finish-line event. Keep following the player's car
 * instead of advancing the autonomous finish camera down the track. */
static s32 s_RetireCameraActive;

/* Retail normally announces FINISHED from the authored finish-line zone and
 * plays cue 0x2B later, when UpdateLapAndFinish advances the race. A fast host
 * frame can cross both conditions together. Starting both special cues on
 * voices 22/23 in one frame makes 0x2B replace FINISHED before it is audible,
 * so retain the retail ordering by waiting for those voices to become idle. */
static s32 s_FinishFollowupCue = -1;

enum {
    FINISH_CUE_SPECIAL_VOICE_GROUP = 4,
    RACE_END_MUSIC_TRACK = 15,
    PAUSE_TOGGLE_DEBOUNCE = 5,
    PAUSE_RESUME_DEBOUNCE = 30,
    PRE_START_SECTOR = -2,
    INITIAL_RACE_TIME = 15000,
    INITIAL_RIVAL_CUE_FLAGS = 0x1FE,
    RACE_SCENE_ID = 12,
    RACE_FRAME_SYNC_THRESHOLD = 0x180,
};

void QueueFinishFollowupCue(s32 cue) {
    s_FinishFollowupCue = cue;
    if (DiagnosticsEnabled("sound_cue_trace")) {
        fprintf(stderr, "rage-port: finish follow-up queued cue=0x%02x\n",
                (unsigned)cue);
    }
}

static void UpdateFinishFollowupCue(void) {
    s32 cue;

    if (s_FinishFollowupCue < 0) {
        return;
    }
    cue = ReleaseFinishFollowupCue(
        &s_FinishFollowupCue,
        SpuGetKeyStatus(
            g_SpecialVoiceBits[FINISH_CUE_SPECIAL_VOICE_GROUP]) != 0);
    if (cue < 0) {
        return;
    }
    if (DiagnosticsEnabled("sound_cue_trace")) {
        fprintf(stderr, "rage-port: finish follow-up released cue=0x%02x\n",
                (unsigned)cue);
    }
    PlaySoundCue(cue);
}

static void UpdateRaceEndState(void) {
    RaceEndFrame frame;

    frame = BuildRaceEndFrame(g_RacePhase, g_GrandPrixMode,
                              g_CourseProgress->retriesRemaining,
                              g_RaceFadeTimer);
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
    }
    if (!frame.advanceTimer) {
        return;
    }
    g_MirrorViewEnabled = 0;
    g_RaceFadeTimer++;
}

static void ToggleRacePause(void) {
    RacePauseToggleResult toggle;

    toggle = DecideRacePauseToggle(
        g_RacePhase, g_RacePaused, (g_PadPressed & PAD_START) != 0,
        g_PauseDebounce, g_GrandPrixMode, g_RaceOptionCursor);
    if (!toggle.toggled) {
        return;
    }

    g_PauseDebounce = PAUSE_TOGGLE_DEBOUNCE;
    g_RacePaused = toggle.paused;
    if (toggle.paused) {
        PauseCdAudio();
        ForceAllEffectVoicesEnabled(0);
        g_RaceOptionCursor = 0;
        PlaySoundCue(2);
        return;
    }

    if (toggle.action == RACE_PAUSE_QUIT) {
        g_RaceFadeTimer = 0;
        g_RacePhase = 7;
        if (g_GrandPrixMode == 0) {
            s32 series = ReadStableRaceSeries();
            s32 course = SeriesCourseIndex();

            g_BestLapTimes[series][course][0] =
                g_RankingRecords[series][course][0].raceTime;
        }
        SeedFinishCamera(&g_PlayerCar);
        StartCdVolumeFade(8);
    } else if (toggle.action == RACE_PAUSE_RETIRE) {
        g_RaceFadeTimer = 0;
        g_RacePhase = 5;
        s_RetireCameraActive = 1;
        if (g_CourseProgress->retriesRemaining != 0) {
            PlaySoundCue(0x3D);
        }
        StartCdVolumeFade(8);
    } else if (toggle.action == RACE_PAUSE_RESTART) {
        ExitRaceScene(0xB);
        g_RacePhase = 8;
    } else {
        g_PauseDebounce = PAUSE_RESUME_DEBOUNCE;
        ForceAllEffectVoicesEnabled(1);
        if (g_RacePhase >= 2) {
            ResumeCdAudio();
        }
    }
}

int RetireCameraActive(void) {
    return s_RetireCameraActive;
}

void EnterRaceScene(void) {
    s32 course;
    s32 series;
    s32 recordMode;
    s32 i;

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
    series = ReadStableRaceSeries();
    recordMode = RaceRecordMode(g_GrandPrixMode);
    g_LapTimeMs = 0;
    g_LapTimeSaturated = 0;
    BuildRaceSectorEnds(g_TrackLength, g_SectorEndDistance);
    g_RefSectorTimes.fields.first = g_BestSectorTimes[series][course][0];
    g_RefSectorTime1 = g_BestSectorTimes[series][course][1];
    g_SectorIndex = PRE_START_SECTOR;
    g_RefSectorTime2 = g_BestSectorTimes[series][course][2];
    /* The retail expression builds a 32-bit address through integer/union
     * arithmetic. On a 64-bit host that truncates the native table pointer.
     * This is the same game lookup expressed with its actual dimensions. */
    g_RefLapTime = g_BestLapTimes[series][course][recordMode];
    g_RaceTimeRemaining = INITIAL_RACE_TIME;
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
    g_CameraViewMode = CAMERA_VIEW_CAR;
    g_RacePhase = 0;
    s_RetireCameraActive = 0;
    s_FinishFollowupCue = -1;
    g_RaceCueFlags = 0;
    g_RivalCueFlags = INITIAL_RIVAL_CUE_FLAGS;
    g_RivalCueCooldown3 = 0;
    g_RivalCueCooldown2 = 0;
    g_RivalCueCooldown1 = 0;
    g_RivalCueCooldown0 = 0;
    InitShuttleScenery();
    SeedFlybyScenery();
    SeedRouteScenery();
    InitPathScenery();
    RequestCdTrack(BgmCdTrack(g_BgmTrack));
    g_PauseDebounce = 0;
    g_RaceFadeTimer = 0;
    InitEffectVoiceRuntime();
    g_RivalCueEnabled = 1;
    g_PlayerAutoSteer = 0;
    g_RaceCueDelay = 0;
    g_SceneId = RACE_SCENE_ID;
    g_FrameSyncThreshold = RACE_FRAME_SYNC_THRESHOLD;
    DrawRoundScreen();
    printf("%s", g_MsgGame0Ok);
}

static void UpdatePausedRaceScene(void) {
    RacePauseCursorResult cursor;
    s32 move;

    SetReverbDepth(0x28, 0x28);
    cursor = MoveRacePauseCursor(
        g_PadPressed, g_RaceOptionCursor, g_GrandPrixMode);
    g_RaceOptionCursor = cursor.cursor;
    for (move = 0; move < cursor.moveCount; move++) {
        PlaySoundCue(1);
    }

    g_SceneTimer--;
    DrawRaceOptionMenu(g_RaceOptionCursor);
    if (g_GrandPrixMode == 0) {
        DrawSplitTimes();
    }
    DrawRaceHudLabels(g_GrandPrixMode);
    if (g_GrandPrixMode != 0) {
        DrawTimeRemaining(g_RaceTimeRemaining);
        DrawRacePosition();
    }
    DrawLapTimes();
    DrawStartCountdown(g_SceneTimer);
    GetTrackZoneBlend(g_PlayerCar.trackProgress);
    DrawPlayerTachometer();

    if ((g_PadHeld &
         RaceCameraButtonMask(g_PadType, g_PadButtonMapping)) &&
        g_CameraViewMode == CAMERA_VIEW_CAR && g_RacePhase == 2) {
        if (g_PadPressed & PAD_R1) {
            g_MirrorViewEnabled = 1;
        } else if (g_PadPressed & PAD_L1) {
            g_MirrorViewEnabled = 0;
        }
    }

    UpdateCamera(g_CameraViewMode,
                 GetCarRenderObject(AsRivalCar(&g_PlayerCar)));
    RequestTrackTexturePage(g_PlayerCar.trackSection);
    if (g_GrandPrixMode != 0) {
        DrawCars();
    }
    if ((g_PlayerCar.facingBackwards != ReadStableRaceSeries()) &&
        WrongWayWarningVisible(g_WrongWayTimer)) {
        DrawWrongWayWarning();
    }
    DrawSkyBackground();
    g_RenderState.envMode4 = g_IsEnvironmentMode4;
    DrawTerrainCells();
    DrawCourseObjects();
    if (g_GrandPrixMode != 0) {
        if (g_GrandPrixClass != 5) {
            DrawStartGridScenery(g_SceneTimer);
        }
        SetLightMatrix(&g_SceneLightMatrix);
        DrawScriptedScenery(0);
        DrawRearViewMirror(g_SceneTimer);
    }
    DrawCourseScenery(SeriesCourseIndex(), g_SceneTimer, 0);
    if (BeginMirrorPass() != 0) {
        DrawCourseScenery(SeriesCourseIndex(), g_SceneTimer, 0);
        EndMirrorPass();
    }
}

static void UpdateActiveRaceScene(void) {
    s32 lapUpdateResult;
    s32 textureSection;
    RaceClockUpdate raceClock;
    RaceStartUpdate raceStart;
    RaceViewSelection raceView;
    WrongWayUpdate wrongWay;

    lapUpdateResult = 0;
    g_AnimTimer++;
    raceClock = UpdateRaceClock(g_RaceTimeRemaining, g_RacePhase,
                                g_GrandPrixMode);
    g_RaceTimeRemaining = raceClock.remaining;

    raceStart = UpdateRaceStartState(g_RacePhase, g_SceneTimer);
    g_RacePhase = raceStart.phase;
    if (raceStart.action == RACE_START_ACTION_UPDATE_INTRO_CAMERA) {
        RunRaceIntroCamera(&g_PlayerCar, g_SceneTimer);
    } else if (raceStart.action == RACE_START_ACTION_BEGIN) {
        BeginCarStandingStart(&g_PlayerCar);
        StartCdAudio();
        g_PauseDebounce = 0x1E;
    }

    if (g_RacePhase < 4) {
        DrawStartCountdown(g_SceneTimer);
        PlayCountdownCues(g_SceneTimer);
    }

    if (g_RacePhase < 5) {
        lapUpdateResult = UpdateLapAndFinish(&g_PlayerCar, g_GrandPrixMode);
        UpdateSplitTimes(&g_PlayerCar, g_GrandPrixMode, lapUpdateResult);
        if (lapUpdateResult < 2) {
            DrawLapTimes();
        }
    }

    if (g_RacePhase < 4) {
        if (g_GrandPrixMode != 0) {
            DrawTimeRemaining(g_RaceTimeRemaining);
        }
        if (raceClock.expired) {
            if (g_CourseProgress->retriesRemaining != 0) {
                PlaySoundCue(0x3D);
            }
            ForceAllEffectVoicesEnabled(0);
            g_RacePhase = 5;
            g_RaceFadeTimer = 0;
            SeedFinishCamera(&g_PlayerCar);
            StartCdVolumeFade(8);
        }
    }

    if (g_GrandPrixMode != 0) {
        if (g_RacePhase < 4) {
            UpdateRacePosition();
            DrawRacePosition();
        }
    }
    if (lapUpdateResult < 2 && g_RacePhase < 5) {
        DrawRaceHudLabels(g_GrandPrixMode);
    }

    if (g_RacePhase > 0) {
        UpdatePlayerCar(&g_PlayerCar);
    } else if (g_RacePhase == 0) {
        UpdateLoadedAudioVoices(0, 0);
    }

    if ((g_RacePhase >= 2) && (g_GrandPrixMode != 0)) {
        UpdateRaceCars();
    }

    if ((g_PadPressed &
         RaceCameraButtonMask(g_PadType, g_PadButtonMapping)) &&
        CanToggleRaceCamera(g_RacePhase)) {
        g_CameraViewMode ^= 1;
    }

    raceView = SelectRaceView(g_RacePhase, s_RetireCameraActive,
                              g_CameraViewMode);
    if (raceView.cameraAction == RACE_CAMERA_ACTION_FINISH) {
        UpdateFinishCamera(&g_PlayerCar);
    } else if (raceView.cameraAction == RACE_CAMERA_ACTION_FOLLOW_PLAYER) {
        UpdateCamera(raceView.cameraView,
                     GetCarRenderObject(AsRivalCar(&g_PlayerCar)));
    }

    textureSection = raceView.useFinishTextureSection
                         ? g_CameraCarTrackSection
                         : g_PlayerCar.trackSection;
    RequestTrackTexturePage(textureSection);

    if (g_GrandPrixMode != 0) {
        DrawCars();
    }
    UpdateEnvironment();
    DrawSkyBackground();

    wrongWay = UpdateWrongWayState(
        g_WrongWayTimer,
        g_PlayerCar.facingBackwards != ReadStableRaceSeries(), g_RacePhase,
        g_SceneTimer);
    g_WrongWayTimer = wrongWay.timer;
    if (wrongWay.drawWarning) {
        DrawWrongWayWarning();
    }
    if (wrongWay.playCue) {
        PlaySoundCue(0x2C);
    }

    g_RenderState.envMode4 = g_IsEnvironmentMode4;
    DrawTerrainCells();
    DrawCourseObjects();
    if (g_GrandPrixMode != 0) {
        if (g_GrandPrixClass != 5) {
            DrawStartGridScenery(g_SceneTimer);
        }
        SetLightMatrix(&g_SceneLightMatrix);
        DrawScriptedScenery(1);
        DrawRearViewMirror(g_SceneTimer);
    }
    DrawCourseScenery(SeriesCourseIndex(), g_SceneTimer, 1);
    if (BeginMirrorPass() != 0) {
        DrawCourseScenery(SeriesCourseIndex(), g_SceneTimer, 0);
        EndMirrorPass();
    }

    GetTrackZoneBlend(g_PlayerCar.trackProgress);
    if (g_RacePhase >= 4) {
        g_ReverbZoneDepth = 0;
    }
    SetReverbDepth(g_ReverbZoneDepth, g_ReverbZoneDepth);
    if ((g_RacePhase != 0) && (lapUpdateResult < 2) &&
        (g_RacePhase < 5)) {
        DrawPlayerTachometer();
    }

    if (g_RacePhase < 4) {
        UpdateZoneAmbience(g_PlayerCar.trackProgress);
        UpdatePointAmbience(g_PlayerCar.trackProgress);
        UpdateTrackEventSound(g_PlayerCar.trackSection);
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
    g_SceneTimer++;
    UpdateFinishFollowupCue();
    if (g_SceneTimer < 0x3D) {
        DrawRoundScreen();
        DrawFullscreenFadeTile(0xFF - ((g_SceneTimer - 6) * 0xB), 0x49);
    }

    if (g_PauseDebounce > 0) {
        g_PauseDebounce--;
    }

    ToggleRacePause();
    UpdateRaceEndState();

    if (g_RacePaused != 0) {
        UpdatePausedRaceScene();
    } else {
        UpdateActiveRaceScene();
    }
}
