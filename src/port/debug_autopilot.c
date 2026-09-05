#include <stdio.h>
#include <string.h>
#include "debug_autopilot.h"
#include "debug_route.h"
#include "debug_gpu_capture.h"
#include "runtime_config.h"
#include "timing_control.h"
#include "game/car.h"
#include "game/diagnostics.h"
#include "game/race.h"
#include "game/player_car_internal.h"
#include "game/track.h"

extern int g_SceneId;
static DebugRoute s_route;
static int s_enabled, s_initialized, s_placed, s_exit, s_laps;
static int s_speed, s_target, s_frames, s_limit;
static int s_targetRaces, s_races, s_lastScene;

static DebugRoutePoint ReadPoint(void *context, int index) {
    const GameTrackPoint *p = TrackPoint(index);
    DebugRoutePoint result = {p->x, p->z, p->angle, p->segmentLength};
    (void)context;
    return result;
}

static int Drive(PlayerCarRuntime *player) {
    GameCarRuntime *car = AsRivalCar(player);
    DebugRouteSample sample;
    CarTrackLimits limits = {0, 0, 0, 0};
    int oldLaps;
    if (g_RacePhase < RACE_PHASE_ACTIVE) return 1;
    if (g_RacePhase > RACE_PHASE_ACTIVE || s_exit) return 0;
    if (g_TrackPoints == NULL || g_TrackPointCount < 2) return 0;
    if (!s_placed) {
        memset(&s_route, 0, sizeof(s_route));
        s_route.point = WrapTrackPointIndex(car->trackPointIndex);
        s_placed = 1;
        fprintf(stderr, "rage-port: autopilot start point=%d course=%d series=%d hz=%d\n",
                s_route.point, g_CourseIndex, g_RaceSeries, TimingBaseHz() / 2);
    }
    oldLaps = s_route.laps;
    if (!DebugRouteAdvance(&s_route, g_TrackPointCount,
                           g_RaceSeries ? 1 : -1, s_speed, TimingBaseHz() / 2,
                           ReadPoint, NULL, &sample)) {
        fprintf(stderr, "rage-port: autopilot result=failed invalid route\n");
        s_exit = 1;
        return 1;
    }
    car->x = sample.x;
    car->z = sample.z;
    car->headingAngle = car->bodyYaw = sample.heading;
    AccumulateLapProgress(car);
    UpdateCarTrackState(car, car->trackPointIndex, &limits);
    CopyCarBodyRotationToModel(car);
    car->modelY = car->y;
    s_laps += s_route.laps - oldLaps;
    if (s_route.laps != oldLaps) {
        fprintf(stderr, "rage-port: autopilot lap=%d target=%d point=%d pos=%d,%d section=%d\n",
                s_laps, s_target, car->trackPointIndex, car->x, car->z, car->trackSection);
        if (!s_targetRaces && s_laps >= s_target) {
            fprintf(stderr, "rage-port: autopilot result=complete laps=%d\n", s_laps);
            s_exit = 1;
        }
    }
    return 1;
}

void DebugAutopilotBeforeScene(void) {
    if (!s_initialized) {
        s_initialized = 1;
        s_enabled = RuntimeConfigEnabled("autopilot.enabled");
        DebugGpuCaptureInit();
        if (!s_enabled) return;
        /* Active races update once per two VBlanks (25/30 Hz), independently
         * of interpolated presentation rate. */
        s_speed = RuntimeConfigInt("autopilot.speed", 4000, 1, 100000);
        s_target = RuntimeConfigInt("autopilot.laps", 3, 1, 100);
        s_targetRaces = RuntimeConfigInt("autopilot.races", 0, 0, 100);
        s_limit = RuntimeConfigInt("autopilot.max_frames", 60000, 1, 1000000);
        fprintf(stderr, "rage-port: DEBUG route autopilot speed=%d laps=%d max_frames=%d (not a physics test)\n",
                s_speed, s_target, s_limit);
    }
    if (!s_enabled) return;
    g_DebugPlayerUpdate = Drive;
    if (s_lastScene == 12 && g_SceneId == 17) {
        ++s_races;
        fprintf(stderr, "rage-port: autopilot race finished=%d target=%d\n",
                s_races, s_targetRaces);
        if (s_targetRaces && s_races >= s_targetRaces) {
            fprintf(stderr, "rage-port: autopilot result=complete races=%d\n", s_races);
            s_exit = 1;
        }
    }
    s_lastScene = g_SceneId;
    if (g_SceneId != 12) s_placed = 0;
    if (!s_exit && ++s_frames >= s_limit) {
        fprintf(stderr, "rage-port: autopilot result=failed frame limit laps=%d\n", s_laps);
        s_exit = 1;
    }
}
int DebugAutopilotShouldExit(void) { return s_exit; }
