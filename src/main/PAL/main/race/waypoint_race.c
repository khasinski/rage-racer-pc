#include "common.h"
#include "game/audio.h"
#include "game/cd.h"
#include "game/race.h"
#include "game/render.h"
#include "game/scratchpad.h"
#include "game/screens.h"
#include "game/state.h"
#include "game/track.h"
#include "game/waypoint.h"
#include "game/player_car_internal.h"


/*
 * Initializes the 6 TrackWaypointRuntime slots for the current course. The
 * `magic`/mult/mfhi block is the compiler's divide-by-10 idiom computing the
 * track index (player lap - 1) / 10, clamped to 0..9, which selects a
 * TrackWaypointSeed row in g_WaypointSeeds. Each slot i is seeded at
 * origin + step*i (x,y), with the fixed constants 0x1766 and 0x174, and marked
 * inactive. Register pins and the raw tail-offset writes are match-load-bearing.
 */

void UpdateWaypointRaceScene(void) {
    s32 value;
    s32 option;
    u32 pausePhase;
    u32 paused;

    option = 0;
    g_SceneTimer++;
    {
        u32 sceneFrame = g_SceneTimer;
        if (sceneFrame < 0x3D) {
            DrawRoundScreen();
            value = g_SceneTimer - 6;
            DrawFullscreenFadeTile(0xFF - (((value * 3) * 4) - value), 0x49);
        }
    }

    if (g_PauseDebounce > 0) {
        g_PauseDebounce--;
    }

    pausePhase = (u16)g_RacePhase - 1;
    if (pausePhase < 2 && (g_PadPressed & PAD_START) && g_PauseDebounce <= 0) {
        g_PauseDebounce = 0;
        paused = g_RacePaused;
        g_RacePaused = paused < 1;

        if (g_RacePaused != 0) {
            PauseCdAudio();
            ForceAllEffectVoicesEnabled(0);
            g_RaceOptionCursor = 0;
            PlaySoundCue(2);
        } else if (g_RaceOptionCursor == 2) {
            g_RaceFadeTimer = 0;
            g_RacePhase = 7;
            StartCdVolumeFade(0x1E);
        } else if (g_RaceOptionCursor == 1) {
            ExitRaceScene(0xB);
        } else {
            ForceAllEffectVoicesEnabled(1);
            if (g_RacePhase >= 2) {
                ResumeCdAudio();
            }
        }
    }

    if (g_RacePhase == 7) {
        value = g_RaceFadeTimer;
        if (value > 0) {
            DrawFullscreenFadeTile(value * 3, 0x49);
            option = 6;
            value = g_RaceFadeTimer;
        }
        if (value >= 0x3D) {
            ExitRaceScene(option);
        }
        g_RaceFadeTimer++;
    }

    if (g_RacePaused != 0) {
        if ((g_PadPressed & PAD_UP) && g_RaceOptionCursor > 0) {
            g_RaceOptionCursor--;
            PlaySoundCue(1);
        }

        if ((g_PadPressed & PAD_DOWN) && g_RaceOptionCursor < (2 - g_GrandPrixMode)) {
            g_RaceOptionCursor++;
            PlaySoundCue(1);
        }

        g_SceneTimer--;
        DrawRaceOptionMenu(g_RaceOptionCursor);
        UpdateFreeLookCamera(&g_PlayerCar, 0);
        GetTrackZoneBlend(g_PlayerCar.trackProgress);
        RequestTrackTexturePage(g_PlayerCar.trackSection);
        DrawSkyBackground();
        SCRATCH_ENV_MODE4 = g_IsEnvironmentMode4;
        DrawTerrainCells();
        DrawCourseObjects();
        DrawFlybyScenery();
        DrawRouteScenery();
        DrawPathScenery();
        DrawCourseScenery(g_CourseIndex & 3, g_SceneTimer, 0);
        return;
    }

    g_AnimTimer++;

    {
        u32 sceneFrame = g_SceneTimer;
        if (sceneFrame >= 0x1F && g_RacePhase == 0) {
            StartCdAudio();
            g_RacePhase = 2;
            g_PauseDebounce = 0xA;
        }
    }

    if (g_CameraCarProgressA >= g_TrackLength && g_RacePhase < 3) {
        g_RaceFadeTimer = 0;
        g_RacePhase = 7;
        StartCdVolumeFade(0x1E);
        ForceAllEffectVoicesEnabled(0);
    }

    UpdateLoadedAudioVoices(0, 1);
    UpdateFreeLookCamera(&g_PlayerCar, 1);
    RequestTrackTexturePage(g_PlayerCar.trackSection);
    UpdateEnvironment();
    DrawSkyBackground();
    SCRATCH_ENV_MODE4 = g_IsEnvironmentMode4;
    DrawTerrainCells();
    DrawCourseObjects();
    UpdateFlybyScenery();
    DrawFlybyScenery();
    UpdateRouteScenery();
    DrawRouteScenery();
    UpdatePathScenerySound();
    DrawPathScenery();
    DrawCourseScenery(g_CourseIndex & 3, g_SceneTimer, 1);
    GetTrackZoneBlend(g_PlayerCar.trackProgress);
    SetReverbDepth(g_ReverbZoneDepth, g_ReverbZoneDepth);
    UpdateZoneAmbience(g_PlayerCar.trackProgress);
    UpdatePointAmbience(g_PlayerCar.trackProgress);
    UpdateTrackEventSound(g_PlayerCar.trackSection);
}

void SeedWaypoints(void) {
    TrackWaypointRuntime *waypoint;
    s32 i;
    TrackWaypointSeed *seed;
    s32 track;

    track = g_PlayerCar.lap - 1;
    track = track % 10;
    if (track < 0) {
        track = 0;
    } else if (track >= 9) {
        track = 9;
    }

    waypoint = g_Waypoints;
    seed = &g_WaypointSeeds[track];

    for (i = 0; i < 6; i++) {
        waypoint->active = 0;
        waypoint->motion.x = seed->x + seed->stepX * i;
        waypoint->motion.y = seed->y + seed->stepY * i;
        waypoint->motion.height = 0x1766;
        waypoint->motion.rotationY = 0x174;
        waypoint->motion.rotationZ = 0;
        waypoint->motion.field1C = 0;
        waypoint++;
    }

    g_WaypointSpawnCooldown = 0;
}
