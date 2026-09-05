#include <assert.h>
#include <string.h>
#include "debug_autopilot.h"
#include "game/car.h"
#include "game/race.h"
#include "game/track.h"

int g_SceneId = 12, g_CourseIndex, g_RaceSeries;
s16 g_RacePhase = RACE_PHASE_ACTIVE;
s32 g_TrackPointCount = 4;
static const GameTrackPoint points[4] = {
    {.x=0,.z=0,.segmentLength=100}, {.x=100,.z=0,.segmentLength=100},
    {.x=100,.z=100,.segmentLength=100}, {.x=0,.z=100,.segmentLength=100}
};
const GameTrackPoint *g_TrackPoints = points;
int (*g_DebugPlayerUpdate)(PlayerCarRuntime *);
static int enabled, timeoutMode, racesMode, updates;
int RuntimeConfigEnabled(const char *key) { (void)key; return enabled; }
int RuntimeConfigInt(const char *key, int fallback, int low, int high) {
    (void)low; (void)high;
    if (!strcmp(key,"autopilot.speed")) return 100;
    if (!strcmp(key,"autopilot.laps")) return 2;
    if (!strcmp(key,"autopilot.races")) return racesMode ? 2 : 0;
    if (!strcmp(key,"autopilot.max_frames")) return timeoutMode ? 2 : 1000;
    return fallback;
}
int TimingBaseHz(void) { return 50; }
void DebugGpuCaptureInit(void) {}
void AccumulateLapProgress(GameCarRuntime *car) { (void)car; updates++; }
s32 UpdateCarTrackState(GameCarRuntime *car, s32 point, const CarTrackLimits *limits) {
    (void)car; (void)point; (void)limits; return 0;
}
int main(int argc, char **argv) {
    PlayerCarRuntime car = {0};
    enabled = argc > 1 && strcmp(argv[1], "disabled");
    timeoutMode = argc > 1 && !strcmp(argv[1], "timeout");
    racesMode = argc > 1 && !strcmp(argv[1], "races");
    DebugAutopilotBeforeScene();
    if (!enabled) {
        assert(g_DebugPlayerUpdate == NULL && !DebugAutopilotShouldExit());
        return 0;
    }
    assert(g_DebugPlayerUpdate != NULL);
    if (timeoutMode) {
        DebugAutopilotBeforeScene();
        assert(DebugAutopilotShouldExit());
        return 0;
    }
    g_RacePhase = RACE_PHASE_INTRO;
    assert(g_DebugPlayerUpdate(&car) == 1 && updates == 0);
    g_RacePhase = RACE_PHASE_FINISHED;
    assert(g_DebugPlayerUpdate(&car) == 0 && updates == 0);
    g_RacePhase = RACE_PHASE_ACTIVE;
    for (int i = 0; i < 200; i++) {
        assert(!DebugAutopilotShouldExit());
        assert(g_DebugPlayerUpdate(&car) == 1);
        DebugAutopilotBeforeScene();
    }
    assert(updates == 200);
    if (racesMode) {
        assert(!DebugAutopilotShouldExit()); /* Tour target must not end a race. */
        g_SceneId = 17;
        DebugAutopilotBeforeScene();
        DebugAutopilotBeforeScene(); /* No double-counting the replay. */
        assert(!DebugAutopilotShouldExit());
        g_SceneId = 19;
        DebugAutopilotBeforeScene();
        g_SceneId = 12;
        DebugAutopilotBeforeScene();
        assert(g_DebugPlayerUpdate(&car) == 1);
        assert(!DebugAutopilotShouldExit());
        g_SceneId = 17;
        DebugAutopilotBeforeScene();
    }
    assert(DebugAutopilotShouldExit());
    return 0;
}
