#include "game/car.h"
#include "game/track.h"
#include "game/race.h"

static s32 WrapTrackPointIndex(s32 index) {
    index %= g_TrackPointCount;
    return index < 0 ? index + g_TrackPointCount : index;
}

static s32 TrackSegmentLength(s32 index) {
    return (s16)g_TrackPoints[index].segmentLength;
}

/* Seeds the launch-spin value from how far the revs sit above the power peak;
 * a car already in gear 2 or higher also starts losing grip. */
void BeginCarStandingStart(PlayerCarRuntime *car) {
    s32 value;
    s32 gear = car->drive.gear;
    s32 revLimit = g_CarSpec->revLimit;

    if (gear < 1) {
        gear = 1;
    } else if (gear > CAR_FORWARD_GEAR_COUNT) {
        gear = CAR_FORWARD_GEAR_COUNT;
    }
    car->drive.gear = (s16)gear;
    if (revLimit <= 0) {
        revLimit = 1;
    }

    value = ((g_EngineRpm - g_PeakOutputRpm) * 10000) / revLimit;
    g_StandingStartState = 0;

    if (value < 0) {
        value = g_EngineRpm - 1000;
        if (g_EngineRpm < 2000) {
            value = 0;
        }
    } else {
        value *= g_PeakOutputValue / ((gear * 200) + 300);
        car->drive.drivetrainTorque /= gear;
        if (gear >= 2) {
            g_GripLossTimer = 200;
        }
    }

    g_StandingStartSpin = value;
}

/*
 * Walks the track-point ring from the event's start point to the car's current
 * point, summing segment lengths into progressA. `mode` picks which way round
 * to walk and the race direction decides the sign of the accumulated distance.
 */
void SeedCarLapProgress(GameCarRuntime *car, s32 mode) {
    s32 current;
    s32 index;
    s32 total = 0;

    if (g_TrackPointCount <= 0 || g_TrackPoints == NULL ||
        g_TrackEventData == NULL) {
        car->progressA = 0;
        return;
    }

    current = WrapTrackPointIndex(car->trackPointIndex);
    index = WrapTrackPointIndex(g_TrackEventData->trackWalkStart);

    if (g_RaceSeries != 0) {
        if (mode == 1) {
            for (;;) {
                index = WrapTrackPointIndex(index + 1);
                if (index == current) {
                    break;
                }
                total += TrackSegmentLength(index);
            }
        } else {
            for (;;) {
                index = WrapTrackPointIndex(index);
                total -= TrackSegmentLength(index);
                if (index == current) {
                    break;
                }
                index--;
            }
        }
    } else {
        if (mode == 0) {
            do {
                index = WrapTrackPointIndex(index + 1);
                total -= TrackSegmentLength(index);
            } while (index != current);
        } else {
            while ((index = WrapTrackPointIndex(index)) != current) {
                total += TrackSegmentLength(index);
                index--;
            }
        }
    }
    car->progressA = total;
}


/*
 * Lap-progress accumulator. Relocates the car's trackPointIndex to the segment
 * that now contains it (FindTrackSegment), then walks the intervening points and
 * adds or subtracts their segmentLength into car->progressA. The race direction
 * controls which physical direction increases lap progress; equal-length paths
 * keep retail's tie-break (backward for series 0, forward otherwise).
 */
void AccumulateLapProgress(GameCarRuntime *car) {
    s32 target;
    s32 current = car->trackPointIndex;
    s32 forwardDistance;
    s32 backwardDistance;
    s32 moveForward;
    s32 i;

    if (g_TrackPointCount <= 0 || g_TrackPoints == NULL) {
        car->activeFlag = -1;
        return;
    }

    current = WrapTrackPointIndex(current);
    target = FindTrackSegment(car, current);
    if (target < 0) {
        car->activeFlag = -1;
        return;
    }
    if (target == current) {
        return;
    }

    forwardDistance = (target - current + g_TrackPointCount) % g_TrackPointCount;
    backwardDistance = (current - target + g_TrackPointCount) % g_TrackPointCount;
    moveForward = forwardDistance < backwardDistance ||
                  (forwardDistance == backwardDistance && g_RaceSeries != 0);

    if (g_RaceSeries == 0) {
        if (moveForward) {
            for (i = 1; i <= forwardDistance; i++) {
                car->progressA -= TrackSegmentLength(
                    WrapTrackPointIndex(current + i));
            }
        } else {
            for (i = 0; i < backwardDistance; i++) {
                car->progressA += TrackSegmentLength(
                    WrapTrackPointIndex(current - i));
            }
        }
    } else {
        if (moveForward) {
            for (i = 0; i < forwardDistance; i++) {
                car->progressA += TrackSegmentLength(
                    WrapTrackPointIndex(current + i));
            }
        } else {
            for (i = 1; i <= backwardDistance; i++) {
                car->progressA -= TrackSegmentLength(
                    WrapTrackPointIndex(current - i));
            }
        }
    }
    car->trackPointIndex = target;
}
