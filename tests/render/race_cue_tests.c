#include "common.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/track.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

PlayerCarRuntime g_PlayerCar;
const TrackEventData *g_TrackEventData;
s32 g_RaceCueFlags;
s32 g_RaceSeries;
s32 g_LapCount;
s16 g_WrongWayTimer;

static s32 g_PlayedCue;
static s32 g_PlayCount;

void PlaySoundCue(s32 cue) {
    g_PlayedCue = cue;
    g_PlayCount++;
}

static void ResetRuntime(void) {
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    g_RaceCueFlags = 0;
    g_WrongWayTimer = 0;
    g_PlayedCue = -1;
    g_PlayCount = 0;
    g_RaceSeries = 0;
    g_LapCount = 3;
    g_PlayerCar.drive.speedScale = 1000;
}

int main(void) {
    TrackEventData events;

    memset(&events, 0, sizeof(events));
    events.raceCues.finish[0].trackSection = 10;
    events.raceCues.finish[1].trackSection = 20;
    events.raceCues.speed[0][0] = (TrackSpeedCue){30, 50};
    events.raceCues.speed[0][1] = (TrackSpeedCue){40, 75};
    events.raceCues.speed[0][2].trackSection = -1;
    events.raceCues.speed[1][0] = (TrackSpeedCue){50, 25};
    events.raceCues.speed[1][1].trackSection = -1;
    g_TrackEventData = &events;

    ResetRuntime();
    g_PlayerCar.trackSection = 10;
    g_PlayerCar.lap = 3;
    TriggerRaceCues();
    if (g_RaceCueFlags != 8 || g_PlayCount != 1 || g_PlayedCue != 0x2A) {
        puts("FAIL: finish cue");
        return 1;
    }
    TriggerRaceCues();
    if (g_PlayCount != 1) {
        puts("FAIL: finish cue repeated");
        return 1;
    }

    ResetRuntime();
    g_PlayerCar.trackSection = 30;
    g_PlayerCar.speed = 501;
    TriggerRaceCues();
    if (g_RaceCueFlags != 0x10 || g_PlayedCue != 0x23) {
        puts("FAIL: speed cue threshold");
        return 1;
    }
    g_PlayCount = 0;
    TriggerRaceCues();
    if (g_PlayCount != 0) {
        puts("FAIL: speed cue repeated");
        return 1;
    }

    ResetRuntime();
    g_PlayerCar.trackSection = 30;
    g_PlayerCar.speed = 500;
    TriggerRaceCues();
    if (g_RaceCueFlags != 0 || g_PlayCount != 0) {
        puts("FAIL: speed cue strict threshold");
        return 1;
    }
    g_PlayerCar.speed = 501;
    g_PlayerCar.motionMode = 1;
    TriggerRaceCues();
    if (g_RaceCueFlags != 0 || g_PlayCount != 0) {
        puts("FAIL: speed cue during car motion");
        return 1;
    }

    ResetRuntime();
    g_PlayerCar.trackSection = 10;
    g_PlayerCar.lap = 3;
    g_WrongWayTimer = 10;
    TriggerRaceCues();
    if (g_RaceCueFlags != 8 || g_PlayCount != 0) {
        puts("FAIL: wrong-way finish suppression");
        return 1;
    }

    ResetRuntime();
    g_RaceSeries = 1;
    g_PlayerCar.trackSection = 50;
    g_PlayerCar.speed = 251;
    TriggerRaceCues();
    if (g_RaceCueFlags != 0x10 || g_PlayedCue != 0x23) {
        puts("FAIL: extra-series speed cue");
        return 1;
    }

    ResetRuntime();
    g_TrackEventData = NULL;
    TriggerRaceCues();
    if (g_PlayCount != 0 || g_RaceCueFlags != 0) {
        puts("FAIL: missing race cue data");
        return 1;
    }

    g_TrackEventData = &events;
    g_RaceSeries = -1;
    TriggerRaceCues();
    g_RaceSeries = TRACK_SERIES_COUNT;
    TriggerRaceCues();
    if (g_PlayCount != 0 || g_RaceCueFlags != 0) {
        puts("FAIL: invalid race cue series");
        return 1;
    }

    ResetRuntime();
    events.raceCues.speed[0][0].speedPercent = INT16_MAX;
    g_PlayerCar.drive.speedScale = INT_MIN;
    g_PlayerCar.trackSection = 30;
    TriggerRaceCues();
    if (g_RaceCueFlags != 0x10 || g_PlayedCue != 0x23) {
        printf("FAIL: wrapped speed threshold flags=%d cue=%d plays=%d\n",
               g_RaceCueFlags, g_PlayedCue, g_PlayCount);
        return 1;
    }

    puts("race cue behavior preserved");
    return 0;
}
