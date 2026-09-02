#include "game/diagnostics.h"
#include <stdio.h>
#include "game/audio.h"
#include "game/input_internal.h"
#include "game/player_car_internal.h"
#include "game/cd.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/random.h"
#include "game/render_internal.h"
#include "game/save_internal.h"
#include "game/race_internal.h"
#include "game/race_scene_internal.h"
#include "game/screens.h"
#include "game/track.h"
#include <stdlib.h>
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
    RACE_END_MUSIC_FRAME = 10,
    RACE_END_FADE_FRAME = 20,
    RACE_END_BANNER_FRAME = 21,
    RACE_END_EXIT_FRAME = 101,
    RACE_RETRY_EXIT_FRAME = 126,
};

void QueueFinishFollowupCue(s32 cue) {
    s_FinishFollowupCue = cue;
    if (DiagnosticsEnabled("sound_cue_trace"))
        fprintf(stderr, "rage-port: finish follow-up queued cue=0x%02x\n",
                (unsigned)cue);
}

static void UpdateFinishFollowupCue(void) {
    s32 cue;
    if (s_FinishFollowupCue < 0 ||
        SpuGetKeyStatus(g_SpecialVoiceBits[4]) != 0) return;
    cue = s_FinishFollowupCue;
    s_FinishFollowupCue = -1;
    if (DiagnosticsEnabled("sound_cue_trace"))
        fprintf(stderr, "rage-port: finish follow-up released cue=0x%02x\n",
                (unsigned)cue);
    PlaySoundCue(cue);
}

static void UpdateRaceEndState(void) {
    RaceEndPresentation presentation;

    if (g_RacePhase == 7) {
        ExitRaceScene(6);
        return;
    }
    if (g_RacePhase != 5) {
        return;
    }

    presentation = ChooseRaceEndPresentation(
        g_GrandPrixMode, g_CourseProgress->retriesRemaining);
    if (presentation == RACE_END_PRESENTATION_FINAL) {
        if (g_RaceFadeTimer >= RACE_END_BANNER_FRAME) {
            s32 fade = (g_RaceFadeTimer - RACE_END_FADE_FRAME) * 3;

            DrawRaceEndBanner(fade);
            DrawFullscreenFadeTile(fade, 0x49);
        }
        if (g_RaceFadeTimer == RACE_END_MUSIC_FRAME) {
            RequestCdTrack(0xF);
            StartCdAudio();
        }
        if (g_RaceFadeTimer >= RACE_END_EXIT_FRAME) {
            ExitRaceScene(0xF);
        }
    } else if (presentation == RACE_END_PRESENTATION_RETRY) {
        DrawLostRaceCaption(g_RaceFadeTimer * 2);
        DrawFullscreenFadeTile(g_RaceFadeTimer * 2, 0x49);
        if (g_RaceFadeTimer >= RACE_RETRY_EXIT_FRAME) {
            ExitRaceScene(0xD);
        }
    }
    g_MirrorViewEnabled = 0;
    g_RaceFadeTimer++;
}

static void ToggleRacePause(void) {
    u16 phase = g_RacePhase;
    u32 paused;
    RacePauseAction action;

    if (!CanPauseRace(phase) || !(g_PadPressed & PAD_START) ||
        g_PauseDebounce > 0) {
        return;
    }

    g_PauseDebounce = 5;
    paused = g_RacePaused < 1;
    g_RacePaused = paused;
    if (paused != 0) {
        PauseCdAudio();
        ForceAllEffectVoicesEnabled(0);
        g_RaceOptionCursor = 0;
        PlaySoundCue(2);
        return;
    }

    action = DecideRacePauseAction(
        phase, g_GrandPrixMode, g_RaceOptionCursor);
    if (action == RACE_PAUSE_QUIT) {
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
    } else if (action == RACE_PAUSE_RETIRE) {
        g_RaceFadeTimer = 0;
        g_RacePhase = 5;
        s_RetireCameraActive = 1;
        if (g_CourseProgress->retriesRemaining != 0) {
            PlaySoundCue(0x3D);
        }
        StartCdVolumeFade(8);
    } else if (action == RACE_PAUSE_RESTART) {
        ExitRaceScene(0xB);
        g_RacePhase = 8;
    } else {
        g_PauseDebounce = 0x1E;
        ForceAllEffectVoicesEnabled(1);
        if (g_RacePhase >= 2) {
            ResumeCdAudio();
        }
    }
}

int RetireCameraActive(void) { return s_RetireCameraActive; }

void EnterRaceScene(void) {
    PlayerCarRuntime *player;
    s32 course;
    s32 series;
    s32 trackLength;
    s32 i;

    SetupDisplay240(0, 0, 0);
    InitRenderState(5);
    ResetReplayWriteCursor();
    ApplyTrackTextureSectionRange();
    InitTrackLighting();
    g_LapCount = RaceLapCount(g_CourseIndex);
    player = &g_PlayerCar;
    InitPlayerCar(player);
    SetTrackTexturePageNow(g_PlayerCar.trackSection);
    BuildStartingGrid();
    trackLength = g_TrackLength;
    course = g_CourseIndex & 3;
    series = ReadStableRaceSeries();
    g_LapTimeMs = 0;
    g_LapTimeSaturated = 0;
    BuildRaceSectorEnds(trackLength, g_SectorEndDistance);
    g_RefSectorTimes.fields.first = g_BestSectorTimes[series][course][0];
    g_RefSectorTime1 = g_BestSectorTimes[series][course][1];
    g_SectorIndex = -2;
    g_RefSectorTime2 = g_BestSectorTimes[series][course][2];
    /* The retail expression builds a 32-bit address through integer/union
     * arithmetic. On a 64-bit host that truncates the native table pointer.
     * This is the same game lookup expressed with its actual dimensions. */
    g_RefLapTime = g_BestLapTimes[series][course][g_GrandPrixMode];
    g_RaceTimeRemaining = 0x3A98;
    g_BestLapThisRace = g_RefLapTime;
    for (i = 0; i < g_LapCount; i++) {
        player->lapTimes.table.frameCounts[i] = 0;
        player->lapTimes.table.milliseconds[i] = 0;
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
    g_RivalCueFlags = 0x1FE;
    g_RivalCueCooldown3 = 0;
    g_RivalCueCooldown2 = 0;
    g_RivalCueCooldown1 = 0;
    g_RivalCueCooldown0 = 0;
    InitShuttleScenery();
    SeedFlybyScenery();
    SeedRouteScenery();
    InitPathScenery();
    RequestCdTrack(g_BgmTrack + 3);
    g_PauseDebounce = 0;
    g_RaceFadeTimer = 0;
    InitEffectVoiceRuntime();
    g_RivalCueEnabled = 1;
    g_PlayerAutoSteer = 0;
    g_RaceCueDelay = 0;
    g_SceneId = 12;
    g_FrameSyncThreshold = 0x180;
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
