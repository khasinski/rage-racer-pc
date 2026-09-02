#include <assert.h>
#include <string.h>

#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_internal.h"

s32 g_SectorIndex;
s32 g_SectorEndDistance[3];
s32 g_SectorTimes[3];
s32 g_LapTimeMs;
s32 g_RefLapTime;
SectorReferenceTimes g_RefSectorTimes;
s16 g_SplitSign;
s32 g_SplitDelta;
s16 g_SplitTimer;
s16 g_SplitSector;
s32 g_SplitTargetTime;
s32 g_BestLapThisRace;
s32 g_LastSectorTime;
s32 g_BestSectorTimes[2][4][3];
s32 g_RaceSeries;
s32 g_LapCount;
s32 g_TrackLength;
s32 g_CourseIndex;

static s32 s_SoundCue;

void PlaySoundCue(s32 cue) { s_SoundCue = cue; }

static void ResetState(void) {
    g_SectorIndex = 0;
    memset(g_SectorEndDistance, 0, sizeof(g_SectorEndDistance));
    memset(g_SectorTimes, 0, sizeof(g_SectorTimes));
    memset(&g_RefSectorTimes, 0, sizeof(g_RefSectorTimes));
    memset(g_BestSectorTimes, 0, sizeof(g_BestSectorTimes));
    g_LapTimeMs = 0;
    g_RefLapTime = 0;
    g_SplitSign = 0;
    g_SplitDelta = 0;
    g_SplitTimer = 0;
    g_SplitSector = 0;
    g_SplitTargetTime = 0;
    g_BestLapThisRace = 0;
    g_LastSectorTime = 0;
    g_RaceSeries = 0;
    g_LapCount = 0;
    g_TrackLength = 1000;
    g_CourseIndex = 2;
    s_SoundCue = 0;
}

static void TestModesThatDoNotHaveSplits(void) {
    PlayerCarRuntime car = {0};

    ResetState();
    UpdateSplitTimes(&car, 1, 0);
    UpdateSplitTimes(&car, 0, 2);
    assert(g_SectorIndex == 0);
}

static void TestInitialLapEvent(void) {
    PlayerCarRuntime car = {0};

    ResetState();
    g_SectorIndex = -2;
    g_RaceSeries = 1;
    g_BestSectorTimes[1][2][0] = 4321;
    UpdateSplitTimes(&car, 0, 1);

    assert(g_SectorIndex == 0);
    assert(g_SplitTargetTime == 4321);
    assert(g_SplitTimer == 0x3C);
    assert(g_SplitSector == 0);
}

static void TestSectorClose(void) {
    PlayerCarRuntime car = {0};

    ResetState();
    car.lap = 1;
    car.progressA = 100;
    g_SectorEndDistance[0] = 100;
    g_LapTimeMs = 900;
    g_RefSectorTimes.values[0] = 1000;
    UpdateSplitTimes(&car, 0, 0);

    assert(g_SectorTimes[0] == 900);
    assert(g_SectorIndex == 1);
    assert(g_SplitSign == 1);
    assert(g_SplitDelta == 100);
    assert(g_SplitTargetTime == 1000);
    assert(g_LastSectorTime == 900);
    assert(s_SoundCue == 0x3E);

    car.progressA = 0;
    car.progressB = 200;
    g_SectorEndDistance[1] = 200;
    g_LapTimeMs = 1100;
    g_RefSectorTimes.values[1] = 1000;
    UpdateSplitTimes(&car, 0, 0);
    assert(g_SectorIndex == 2);
    assert(g_SplitSign == -1);
    assert(g_SplitDelta == 100);
    assert(s_SoundCue == 0x3F);
}

static void TestSplitDisplayExpiry(void) {
    PlayerCarRuntime car = {0};

    ResetState();
    car.lap = 1;
    g_LapCount = 1;
    g_SectorEndDistance[0] = 500;
    g_RefSectorTimes.values[0] = 1234;
    g_SplitTimer = 59;
    g_SplitSign = -1;

    UpdateSplitTimes(&car, 0, 0);

    assert(g_SplitTimer == 60);
    assert(g_SplitTargetTime == 1234);
    assert(g_SplitSign == 0 && g_SplitSector == 0);
}

static void TestInactiveLapResetsSplit(void) {
    PlayerCarRuntime car = {0};

    ResetState();
    car.lap = 1;
    g_SectorIndex = 1;
    g_SectorEndDistance[1] = 500;
    g_RefSectorTimes.values[0] = 4321;
    g_SplitTimer = 12;
    g_SplitSign = -1;

    UpdateSplitTimes(&car, 0, 0);

    assert(g_SplitSector == 0 && g_SplitTimer == 0 && g_SplitSign == 0);
    assert(g_SplitTargetTime == 4321);
}

static void TestUnrepresentableTimeHasNoDelta(void) {
    PlayerCarRuntime car = {0};

    ResetState();
    car.lap = 1;
    car.progressA = 100;
    g_SectorEndDistance[0] = 100;
    g_LapTimeMs = 599999;
    g_SplitSign = -1;

    UpdateSplitTimes(&car, 0, 0);

    assert(g_SectorTimes[0] == 599999);
    assert(g_SplitSign == 0 && s_SoundCue == 0);
}

int main(void) {
    TestModesThatDoNotHaveSplits();
    TestInitialLapEvent();
    TestSectorClose();
    TestSplitDisplayExpiry();
    TestInactiveLapResetsSplit();
    TestUnrepresentableTimeHasNoDelta();
    return 0;
}
