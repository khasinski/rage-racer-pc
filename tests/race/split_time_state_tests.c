#include <assert.h>
#include <limits.h>
#include <string.h>

#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_hud_internal.h"
#include "game/race_internal.h"

static RaceTiming s_timing;
s32 g_BestSectorTimes[2][4][3];
s32 g_RaceSeries;
s32 g_LapCount;
s32 g_TrackLength;
s32 g_CourseIndex;

static s32 s_SoundCue;

void PlaySoundCue(s32 cue) { s_SoundCue = cue; }

static void ResetState(void) {
    memset(&s_timing, 0, sizeof(s_timing));
    memset(g_BestSectorTimes, 0, sizeof(g_BestSectorTimes));
    s_timing.lapTime = 0;
    s_timing.bestLap = 0;
    g_RaceSeries = 0;
    g_LapCount = 0;
    g_TrackLength = 1000;
    g_CourseIndex = 2;
    s_SoundCue = 0;
}

static void TestModesThatDoNotHaveSplits(void) {
    PlayerCarRuntime car = {0};

    ResetState();
    UpdateSplitTimes(&s_timing, &car, 1, 0);
    UpdateSplitTimes(&s_timing, &car, 0, 2);
    assert(s_timing.sectorIndex == 0);
}

static void TestInitialLapEvent(void) {
    PlayerCarRuntime car = {0};

    ResetState();
    s_timing.sectorIndex = -2;
    g_RaceSeries = 1;
    g_BestSectorTimes[1][2][0] = 4321;
    UpdateSplitTimes(&s_timing, &car, 0, 1);

    assert(s_timing.sectorIndex == 0);
    assert(s_timing.splitTargetTime == 4321);
    assert(s_timing.splitTimer == 0x3C);
    assert(s_timing.splitSector == 0);
}

static void TestPreStartWaitsForStartLine(void) {
    PlayerCarRuntime car = {0};

    ResetState();
    s_timing.sectorIndex = -2;
    s_timing.splitTargetTime = 98765;
    UpdateSplitTimes(&s_timing, &car, 0, 0);

    assert(s_timing.sectorIndex == -2);
    assert(s_timing.splitTargetTime == 98765);
}

static void TestBlankReferenceDoesNotCreateDelta(void) {
    PlayerCarRuntime car = {0};

    ResetState();
    car.lap = 1;
    car.progressA = 100;
    s_timing.sectorEnds[0] = 100;
    s_timing.lapTime = 900;
    s_timing.refSectorTimes.values[0] = 0;
    s_timing.splitSign = -1;
    UpdateSplitTimes(&s_timing, &car, 0, 0);

    assert(s_timing.sectorTimes[0] == 900);
    assert(s_timing.sectorIndex == 1);
    assert(s_timing.splitSign == 0);
    assert(s_SoundCue == 0);
}

static void TestSectorClose(void) {
    PlayerCarRuntime car = {0};

    ResetState();
    car.lap = 1;
    car.progressA = 100;
    s_timing.sectorEnds[0] = 100;
    s_timing.lapTime = 900;
    s_timing.refSectorTimes.values[0] = 1000;
    UpdateSplitTimes(&s_timing, &car, 0, 0);

    assert(s_timing.sectorTimes[0] == 900);
    assert(s_timing.sectorIndex == 1);
    assert(s_timing.splitSign == 1);
    assert(s_timing.splitDelta == 100);
    assert(s_timing.splitTargetTime == 1000);
    assert(s_timing.lastSectorTime == 900);
    assert(s_SoundCue == 0x3E);

    car.progressA = 0;
    car.progressB = 200;
    s_timing.sectorEnds[1] = 200;
    s_timing.lapTime = 1100;
    s_timing.refSectorTimes.values[1] = 1000;
    UpdateSplitTimes(&s_timing, &car, 0, 0);
    assert(s_timing.sectorIndex == 2);
    assert(s_timing.splitSign == -1);
    assert(s_timing.splitDelta == 100);
    assert(s_SoundCue == 0x3F);
}

static void TestSplitDisplayExpiry(void) {
    PlayerCarRuntime car = {0};

    ResetState();
    car.lap = 1;
    g_LapCount = 1;
    s_timing.sectorEnds[0] = 500;
    s_timing.refSectorTimes.values[0] = 1234;
    s_timing.splitTimer = 59;
    s_timing.splitSign = -1;

    UpdateSplitTimes(&s_timing, &car, 0, 0);

    assert(s_timing.splitTimer == 60);
    assert(s_timing.splitTargetTime == 1234);
    assert(s_timing.splitSign == 0 && s_timing.splitSector == 0);
}

static void TestInactiveLapResetsSplit(void) {
    PlayerCarRuntime car = {0};

    ResetState();
    car.lap = 1;
    s_timing.sectorIndex = 1;
    s_timing.sectorEnds[1] = 500;
    s_timing.refSectorTimes.values[0] = 4321;
    s_timing.splitTimer = 12;
    s_timing.splitSign = -1;

    UpdateSplitTimes(&s_timing, &car, 0, 0);

    assert(s_timing.splitSector == 0 && s_timing.splitTimer == 0 && s_timing.splitSign == 0);
    assert(s_timing.splitTargetTime == 4321);
}

static void TestUnrepresentableTimeHasNoDelta(void) {
    PlayerCarRuntime car = {0};

    ResetState();
    car.lap = 1;
    car.progressA = 100;
    s_timing.sectorEnds[0] = 100;
    s_timing.lapTime = SPLIT_TIME_MAX_MS + 1;
    s_timing.splitSign = -1;

    UpdateSplitTimes(&s_timing, &car, 0, 0);

    assert(s_timing.sectorTimes[0] == SPLIT_TIME_MAX_MS + 1);
    assert(s_timing.splitSign == 0 && s_SoundCue == 0);
}

static void TestInvalidStateIsContained(void) {
    PlayerCarRuntime car = {0};

    ResetState();
    s_timing.sectorIndex = INT_MAX;
    s_timing.splitSign = -1;
    s_timing.splitTimer = 12;
    s_timing.refSectorTimes.values[0] = 321;
    UpdateSplitTimes(&s_timing, &car, 0, 0);
    assert(s_timing.sectorIndex == 0 && s_timing.splitSector == 0);
    assert(s_timing.splitSign == 0 && s_timing.splitTimer == 0);
    assert(s_timing.splitTargetTime == 321);

    s_timing.sectorIndex = 2;
    UpdateSplitTimes(&s_timing, NULL, 0, 0);
    assert(s_timing.sectorIndex == 2);
}

static void TestExtremeArithmeticSaturates(void) {
    PlayerCarRuntime car = {0};

    ResetState();
    car.lap = SHRT_MIN;
    car.progressA = INT_MAX;
    car.progressB = INT_MAX;
    g_TrackLength = INT_MAX;
    s_timing.sectorEnds[0] = INT_MIN;
    s_timing.lapTime = 0;
    s_timing.refLapTime = INT_MAX;
    UpdateSplitTimes(&s_timing, &car, 0, 1);
    assert(s_timing.splitSign == 0);

    ResetState();
    car.lap = 1;
    car.progressA = 100;
    s_timing.sectorEnds[0] = 100;
    s_timing.lapTime = -1;
    UpdateSplitTimes(&s_timing, &car, 0, 0);
    assert(s_timing.splitSign == 0);
}

int main(void) {
    TestModesThatDoNotHaveSplits();
    TestInitialLapEvent();
    TestPreStartWaitsForStartLine();
    TestBlankReferenceDoesNotCreateDelta();
    TestSectorClose();
    TestSplitDisplayExpiry();
    TestInactiveLapResetsSplit();
    TestUnrepresentableTimeHasNoDelta();
    TestInvalidStateIsContained();
    TestExtremeArithmeticSaturates();
    return 0;
}
