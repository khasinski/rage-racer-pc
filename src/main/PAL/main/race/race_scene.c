#include "common.h"
#include "game/game_input.h"
#include <stdio.h>
#include "game/audio.h"
#include "game/car.h"
#include "game/input_internal.h"
#include "game/player_car_internal.h"
#include "game/cd.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/race_clock.h"
#include "game/lap_tracker.h"
#include "game/race_pause.h"
#include "game/race_end.h"
#include "game/race_session.h"
#include "game/race_session_state.h"
#include "game/race_session_runtime.h"
#include "game/race_result.h"
#include "game/race_result_runtime.h"
#include "game/random.h"
#include "game/records_internal.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/save_internal.h"
#include "game/race_internal.h"
#include "game/render_workspace.h"
#include "game/screens.h"
#include "game/state.h"
#include "game/game_context.h"
#include "game/track.h"
#include "psyq/gte.h"

/* A retirement is not a finish-line event. Keep following the player's car
 * instead of advancing the autonomous finish camera down the track. */
static s32 s_RetireCameraActive;

static void ApplyRaceTimingState(const RaceSessionState *state) {
    g_LapCount = state->lapCount;
    g_LapTimeMs = state->lapTimeMs;
    g_LapTimeSaturated = state->lapTimeSaturated;
    g_SectorEndDistance[0] = state->sectorEndDistance[0];
    g_SectorEndDistance[1] = state->sectorEndDistance[1];
    g_SectorEndDistance[2] = state->sectorEndDistance[2];
    g_SectorIndex = state->sectorIndex;
    g_RaceTimeRemaining = state->raceTimeRemaining;
}

static void ApplyRaceRuntimeState(const RaceSessionState *state) {
    g_AnimTimer = state->animTimer;
    g_SceneTimer = state->sceneTimer;
    g_CameraViewMode = state->cameraViewMode;
    g_RacePhase = state->racePhase;
    g_RaceCueFlags = state->raceCueFlags;
    g_RivalCueFlags = state->rivalCueFlags;
    g_RivalCueCooldown0 = state->rivalCueCooldown[0];
    g_RivalCueCooldown1 = state->rivalCueCooldown[1];
    g_RivalCueCooldown2 = state->rivalCueCooldown[2];
    g_RivalCueCooldown3 = state->rivalCueCooldown[3];
}

static void ApplyRacePostSetupState(const RaceSessionState *state) {
    g_PauseDebounce = state->pauseDebounce;
    g_RaceFadeTimer = state->raceFadeTimer;
}

static void ApplyRaceControlState(const RaceSessionState *state) {
    g_RivalCueEnabled = state->rivalCueEnabled;
    g_PlayerAutoSteer = state->playerAutoSteer;
    g_RaceCueDelay = state->raceCueDelay;
    s_RetireCameraActive = state->retireCameraActive;
}

int RageRetireCameraActive(void) { return s_RetireCameraActive; }

/* The first union field and the two trailing split symbols keep separate
 * %hi/%lo accesses. Indexing the union here makes GCC 2.6.3 CSE its base and
 * shifts the allocation of the surrounding block. */

/*
 * Optional trace for the state returned by the lap/finish update. A null
 * format disables it; otherwise the six named values are the complete
 * printf argument list.
 */
static __inline__ void GameDebugLapResult(
    char *format,
    s32 result,
    s32 progress,
    s32 mode,
    s32 lapCount,
    s32 racePhase,
    s32 fadeTimer)
{
    if (format != 0) {
        printf(
            format, result, progress, mode, lapCount, racePhase, fadeTimer);
    }
}


s32 UpdateLapAndFinish(PlayerCarRuntime *car, s32 grandPrixMode) {
    s32 value;
    s32 result;
    s16 recordIndex;
    s32 candidateTime;
    u16 returnValue;
    s16 progress;
    s32 step;
    s32 routeProgress;
    s32 oldTimer;
    s32 timer;
    PlayerCarRaceState *route;
    RaceLapClock lapClock;
    LapTrackerInput lapTrackerInput;
    LapTrackerDecision lapDecision;
    RaceResult raceResult;

    route = GetPlayerCarRaceState(car);
    if (route->timing.fields.lap > 0) {
        if (g_LapCount >= route->timing.fields.lap) {
            lapClock = RaceClockTickLap(
                route->timing.fields.lapTimes.table
                    .frameCounts[route->timing.fields.lap - 1],
                Random15() % 40);
            route->timing.fields.lapTimes.table
                .frameCounts[route->timing.fields.lap - 1] =
                lapClock.frameCount;
            route->timing.fields.lapTimes.table
                .milliseconds[route->timing.fields.lap - 1] =
                lapClock.milliseconds;
            if (lapClock.saturated) g_LapTimeSaturated = 1;
            g_LapTimeMs = route->timing.fields.lapTimes.table
                              .milliseconds[route->timing.fields.lap - 1];
            goto timing_done;
        }
    }
    if (g_LapCount < route->timing.fields.lap) {
        if (g_RaceTotalTime <
            g_BestTotalTimes[ReadStableRaceSeries()][RageSeriesCourseIndex()][grandPrixMode]) {
            g_BestTotalTimes[ReadStableRaceSeries()][RageSeriesCourseIndex()][grandPrixMode] = g_RaceTotalTime;
        }
    }

timing_done:
    progress = route->timing.fields.lap;
    lapTrackerInput = (LapTrackerInput){
        progress, g_LapCount,
        g_PlayerCar.progressB + g_PlayerCar.progressA, g_TrackLength};
    lapDecision = LapTrackerEvaluate(&lapTrackerInput);
    if (lapDecision.crossedLine) {
        returnValue = 1;
        route->timing.fields.lap = lapDecision.nextLap;
        g_LapTimeSaturated = 0;
        g_RaceCueFlags &= 0xF;
        if (g_RaceCueDelay == 0) {
            g_RaceCueDelay = 2;
        }
        recordIndex = route->timing.fields.lap;
        candidateTime = route->timing.fields.lapTimes.table
                            .milliseconds[recordIndex - 2];
        step = candidateTime < g_BestLapThisRace;
        if (step && (recordIndex != 1)) {
            routeProgress = (u16)route->timing.fields.lap;
            route->drive.hudLapHighlightRow = routeProgress - 2;
            result = route->timing.fields.lapTimes.table
                         .milliseconds[route->timing.fields.lap - 2];
            g_BestLapThisRace = candidateTime;
            g_SectorTimes[2] = result;
            if (grandPrixMode == 0) {
                g_RefSectorTime2 = result;
                g_RefSectorTimes.fields.first = g_SectorTimes[0];
                g_RefSectorTime1 = g_SectorTimes[1];
            }

            if (!(g_LapCount < route->timing.fields.lap)) {
                PlaySoundCue(0x26);
                g_RaceCueDelay = 0x96;
            }
        }

        raceResult = RaceResultFromFinish(
            lapDecision.finished, route->drive.racePosition);
        ApplyRaceFinishResult(route, g_LapCount, grandPrixMode, raceResult);
    } else {
        returnValue = 0;
    }

    if ((g_LapCount < route->timing.fields.lap) &&
        (g_RacePhase == 4)) {
        DrawFullscreenFadeTile(g_RaceFadeTimer * 2, 0x29);
        timer = g_RaceFadeTimer;
        oldTimer = timer;
        timer = timer < 2;
        if (!timer) {
            returnValue = 2;
        }
        timer = oldTimer + 1;
        g_RaceFadeTimer = timer;
        if ((s16)timer == 0x3F) {
            if (g_GrandPrixMode != 0) {
                CommitClassProgress();
                if (g_SeriesCleared == 1) {
                    RequestCdTrack(0x10);
                } else {
                    RequestCdTrack(0xC);
                }
            } else {
                g_SeriesCleared = 0;
                RequestCdTrack(0xD);
            }
        }
        if (g_RaceFadeTimer >= 0x83) {
            BeginReplay();
            ExitRaceScene(0x11);
            StartCdAudio();

        }
    } else if ((g_GrandPrixMode == 0) &&
               (((car->progressB + car->progressA) <= -g_TrackLength) ||
                ((g_PlayerCar.lap == 0) && (g_WrongWayTimer >= 0x3C)))) {
        g_RacePhase = 5;
        g_BestLapTimes[ReadStableRaceSeries()][RageSeriesCourseIndex()][0] =
            g_RankingRecords[ReadStableRaceSeries()][RageSeriesCourseIndex()][0].raceTime;
        StartCdVolumeFade(8);
        ForceAllEffectVoicesEnabled(0);
        g_RaceFadeTimer = 0;
        SeedFinishCamera(&g_PlayerCar);
    }

    if (g_RaceCueDelay == 2) {
        value = g_LapCount - route->timing.fields.lap;
        switch (value) {
        case 2:
            PlaySoundCue(0x27);
            break;
        case 1:
            PlaySoundCue(0x28);
            break;
        case 0:
            PlaySoundCue(0x29);
            break;
        }
        g_RaceCueDelay--;
    } else if (g_RaceCueDelay == 1) {
        g_RaceCueDelay = 0;
        g_RivalCueEnabled = 2;
    } else if (g_RaceCueDelay > 0) {
        g_RaceCueDelay--;
    }

    UpdateRivalCueGate();
    GameDebugLapResult(
        0, returnValue, progress, grandPrixMode, g_LapCount,
        g_RacePhase, g_RaceFadeTimer);
    return returnValue;
}

void EnterRaceScene(void) {
    s32 pad[2];
    PlayerCarRuntime *player;
    s32 mode;
    s32 scene;
    s32 tableOffset;
    s32 count;
    s32 i;
    s32 *first;
    s32 *second;
    SectorTimeTableAddress sectorAddress;
    SectorTimeTableAddress lastSectorAddress;
    RaceSessionState sessionState;

    SetupDisplay240(0, 0, 0);
    InitRenderState(5);
    ResetReplayWriteCursor();
    LoadTrackTexturePageRange();
    InitTrackLighting();
    g_TrackWalkStart = g_TrackEventData->trackWalkStart;
    RaceSessionStateReset(&sessionState, g_CourseIndex, g_TrackLength);
    g_LapCount = sessionState.lapCount;
    player = &g_PlayerCar;
    InitPlayerCar(player);
    SetTrackTexturePageNow(g_PlayerCar.trackSection);
    BuildStartingGrid();
    count = g_CourseIndex;
    mode = count & 3;
    scene = ReadStableRaceSeries();
    ApplyRaceTimingState(&sessionState);
    tableOffset = (mode * 12) + (scene * 48);
    sectorAddress.table = g_BestSectorTimes;
    sectorAddress.bytes += tableOffset;
    g_RefSectorTimes.fields.first = sectorAddress.pointer[0];
    sectorAddress.table = g_BestSectorTimes;
    sectorAddress.bytes += tableOffset;
    g_RefSectorTime1 = sectorAddress.pointer[1];
    lastSectorAddress.table = g_BestSectorTimes;
    lastSectorAddress.bytes += tableOffset;
    g_RefSectorTime2 = lastSectorAddress.pointer[2];
    /* The retail expression builds a 32-bit address through integer/union
     * arithmetic. On a 64-bit host that truncates the native table pointer.
     * This is the same game lookup expressed with its actual dimensions. */
    g_RefLapTime =
        g_BestLapTimes[ReadStableRaceSeries()][RageSeriesCourseIndex()][g_GrandPrixMode];
    count = g_LapCount;
    g_BestLapThisRace = g_RefLapTime;
    if (count > 0) {
        i = 0;
        second = player->lapTimes.table.milliseconds;
        first = player->lapTimes.table.frameCounts;
        do {
            *first = 0;
            *second = 0;
            second++;
            i++;
            first++;
        } while (i < count);
    }
    g_RaceTotalTime = sessionState.raceTotalTime;
    ResetMirrorState();
    SeekEnvironmentScript(g_TrackRenderTable->environmentScriptOffset);
    BuildTileStrips();
    BuildRaceHudPrims(g_GrandPrixMode);
    ApplyRaceRuntimeState(&sessionState);
    ResetFreeLookCamera();
    InitShuttleScenery();
    SeedFlybyScenery();
    SeedRouteScenery();
    InitPathScenery();
    RequestCdTrack(g_BgmTrack + 3);
    ApplyRacePostSetupState(&sessionState);
    InitEffectVoiceRuntime();
    ApplyRaceControlState(&sessionState);
    do {
    } while (0);
    GameSceneSet(SCENE_RACE);
    g_FrameSyncThreshold = sessionState.frameSyncThreshold;
    DrawRoundScreen();
    printf("%s", g_MsgGame0Ok);

    (void)pad;
}

void UpdateRaceScene(void) {
    s32 option;
    s32 value;
    u32 timerValue;
    s32 next;
    RaceSession session;
    RaceSessionCommands sessionCommands;

    value = g_SceneTimer + 1;
    g_SceneTimer = value;
    option = 0;
    timerValue = value;
    if (timerValue < 0x3D) {
        DrawRoundScreen();
        DrawFullscreenFadeTile(0xFF - ((g_SceneTimer - 6) * 0xB), 0x49);
    }

    CaptureRaceSession(&session, s_RetireCameraActive);
    RaceSessionStep(&session, g_GameInput.pressed, &sessionCommands);
    ApplyRaceSession(&session, &sessionCommands, &s_RetireCameraActive);

    if (g_RacePaused != 0) {
        if (sessionCommands.pause.setPauseReverb)
            SetReverbDepth(0x28, 0x28);
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

        {
            s32 selectorMask;
            u16 inputMask;

            selectorMask = g_GameInput.controllerType;
            inputMask = g_GameInput.held;
            selectorMask = selectorMask == 0x23;
            if ((inputMask & g_PadButtonMapping[6 + selectorMask * 8]) &&
                g_CameraViewMode == CAMERA_VIEW_CAR && g_RacePhase == 2) {
                if (g_GameInput.pressed & 8) {
                    g_MirrorViewEnabled = 1;
                } else if (g_GameInput.pressed & 4) {
                    g_MirrorViewEnabled = 0;
                }
            }
        }

        UpdateCamera(g_CameraViewMode, (GameRenderObject *)&g_PlayerCar);
        RequestTrackTexturePage(g_PlayerCar.trackSection);
        if (g_GrandPrixMode != 0) {
            DrawCars();
        }
        if ((g_PlayerFacingBackwards != ReadStableRaceSeries()) && (g_WrongWayTimer >= 0xA)) {
            DrawWrongWayWarning();
        }
        DrawSkyBackground();
        RENDER_ENV_MODE4 = g_IsEnvironmentMode4;
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
        DrawCourseScenery(RageSeriesCourseIndex(), g_SceneTimer, 0);
        if (BeginMirrorPass() != 0) {
            DrawCourseScenery(RageSeriesCourseIndex(), g_SceneTimer, 0);
            EndMirrorPass();
        }
    } else {
        u32 frameValue;

        g_AnimTimer++;
        if ((g_RacePhase >= 2) && (g_GrandPrixMode != 0)) {
            g_RaceTimeRemaining = RaceClockTickCountdown(
                g_RaceTimeRemaining, 1);
        }

        frameValue = g_SceneTimer;
        if (frameValue >= 0x5A) {
            if (g_RacePhase == 0) {
                g_RacePhase = 1;
            } else {
                goto update_race;
            }
        } else if (g_RacePhase == 0) {
            RunRaceIntroCamera(&g_PlayerCar, frameValue);
        } else {
update_race:
            if ((g_RacePhase == 1) && (g_SceneTimer >= 0xD3)) {
                BeginCarStandingStart(&g_PlayerCar, frameValue);
                StartCdAudio();
                g_RacePhase = 2;
                g_PauseDebounce = 0x1E;
            }
        }

        if (g_RacePhase < 4) {
            DrawStartCountdown(g_SceneTimer);
            PlayCountdownCues(g_SceneTimer);
        }

        if (g_RacePhase < 5) {
            option = UpdateLapAndFinish(&g_PlayerCar, g_GrandPrixMode);
            UpdateSplitTimes(&g_PlayerCar, g_GrandPrixMode, option);
            if (option < 2) {
                DrawLapTimes();
            }
        }

        if (g_RacePhase < 4) {
            if (g_GrandPrixMode != 0) {
                DrawTimeRemaining(g_RaceTimeRemaining);
            }
            if (g_RaceTimeRemaining <= 0) {
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
        if (option < 2 && g_RacePhase < 5) {
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

        {
            s32 selectorMask;
            u16 inputMask;

            selectorMask = g_GameInput.controllerType;
            inputMask = g_GameInput.pressed;
            selectorMask = selectorMask == 0x23;
            if ((inputMask & g_PadButtonMapping[6 + selectorMask * 8]) &&
                (u32)((u16)g_RacePhase - 2) < 2U) {
                g_CameraViewMode ^= 1;
            }
        }

        if (g_RacePhase == 5 && !s_RetireCameraActive) {
            UpdateFinishCamera(&g_PlayerCar);
        } else if (g_RacePhase > 0) {
            UpdateCamera(s_RetireCameraActive ? CAMERA_VIEW_CAR : g_CameraViewMode,
                         (GameRenderObject *)&g_PlayerCar);
        }

        if (g_RacePhase != 5 || s_RetireCameraActive) {
            next = g_PlayerCar.trackSection;
        } else {
            next = g_CameraCarTrackSection;
        }
        RequestTrackTexturePage(next);

        if (g_GrandPrixMode != 0) {
            DrawCars();
        }
        UpdateEnvironment();
        DrawSkyBackground();

        if ((g_PlayerFacingBackwards != ReadStableRaceSeries()) && (g_RacePhase < 4)) {
            s16 counter;

            counter = g_WrongWayTimer + 1;
            g_WrongWayTimer = counter;
            if (counter >= 0xA) {
                DrawWrongWayWarning();
                if (g_WrongWayTimer >= 0x51) {
                    g_WrongWayTimer = 0xA;
                }
                if ((u8)g_SceneTimer == 0) {
                    PlaySoundCue(0x2C);
                }
            }
        } else {
            g_WrongWayTimer = 0;
        }

        RENDER_ENV_MODE4 = g_IsEnvironmentMode4;
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
        DrawCourseScenery(RageSeriesCourseIndex(), g_SceneTimer, 1);
        if (BeginMirrorPass() != 0) {
            DrawCourseScenery(RageSeriesCourseIndex(), g_SceneTimer, 0);
            EndMirrorPass();
        }

        GetTrackZoneBlend(g_PlayerCar.trackProgress);
        if (g_RacePhase >= 4) {
            g_ReverbZoneDepth = 0;
        }
        SetReverbDepth(g_ReverbZoneDepth, g_ReverbZoneDepth);
        if ((g_RacePhase != 0) && (option < 2) && (g_RacePhase < 5)) {
            DrawPlayerTachometer();
        }

        if (g_RacePhase < 4) {
            s32 *valuePtr;

            valuePtr = &g_PlayerCar.trackProgress;
            UpdateZoneAmbience(*valuePtr);
            UpdatePointAmbience(*valuePtr);
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

}
