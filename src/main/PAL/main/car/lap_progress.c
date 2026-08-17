#include "common.h"
#include "game/car.h"
#include "game/track.h"
#include "game/race.h"

/* Seeds the launch-spin value from how far the revs sit above the power peak;
 * a car already in gear 2 or higher also starts losing grip. */
void BeginCarStandingStart(PlayerCarRuntime *car, s32 sceneTimer) {
    s32 value;
    s16 index;
    (void)sceneTimer;

    value = ((g_EngineRpm - g_PeakOutputRpm) * 10000) / g_CarSpec->revLimit;
    g_StandingStartState = 0;

    if (value < 0) {
        value = g_EngineRpm - 1000;
        if (g_EngineRpm < 2000) {
            value = 0;
        }
    } else {
        index = car->drive.gear;
        value *= g_PeakOutputValue / ((index * 200) + 300);
        car->drive.drivetrainTorque = car->drive.drivetrainTorque / index;
        if (car->drive.gear >= 2) {
            g_GripLossTimer = 200;
        }
    }

    g_StandingStartSpin = value;
}

/*
 * Walks the track-point ring from the event's start point to the car's current
 * point, summing segment lengths into progressA. `mode` picks which way round
 * to walk; the backwards arm then reuses the parameter as its own cursor.
 */
void SeedCarLapProgress(GameCarRuntime *car, s32 mode) {
    GameCarRuntime *obj = car;
    s32 state = g_RaceSeries;
    s32 cur = obj->trackPointIndex;
    s32 total = 0;
    s32 index;

    obj->progressA = 0;
    if (state != 0) {
        index = g_TrackEventData->trackWalkStart;
        if (mode == 1) {
            s32 count;
            GameTrackPoint *table;
            s32 wrapped;

            count = g_TrackPointCount;
            table = g_TrackPoints;
while (1) {
            index++;
            wrapped = index % count;
            if (cur == wrapped) {
                break;
            }
            total += (s16)table[wrapped].segmentLength;
            }
        } else {
            s32 count;
            GameTrackPoint *table;
            s32 wrapped;
            s32 mod;

            count = g_TrackPointCount;
            table = g_TrackPoints;
while (1) {
            if (index < 0) {
                wrapped = index + count;
            } else {
                wrapped = index;
            }
            mod = wrapped % count;
            total -= (s16)table[mod].segmentLength;
            if (cur == wrapped) {
                break;
            }
            index--;
            }
        }
    } else {
        index = g_TrackEventData->trackWalkStart;
        if (mode == 0) {
            s32 count;
            GameTrackPoint *table;
            s32 wrapped;

            count = g_TrackPointCount;
            table = g_TrackPoints;
            do {
                index++;
                wrapped = index % count;
                total -= (s16)table[wrapped].segmentLength;
            } while (cur != wrapped);

        } else {
            s32 count;
            GameTrackPoint *table;
            s32 mod;

            count = g_TrackPointCount;
            table = g_TrackPoints;
            do {
                if (index < 0) {
                    mode = index + count;
                } else {
                    mode = index;
                }
                if (cur == mode) {
                    break;
                }
                mod = mode % count;
                total += (s16)table[mod].segmentLength;
                index--;
            } while (1);
        }
    }
    obj->progressA = total;
}


/*
 * Lap-progress accumulator. Relocates the car's trackPointIndex to the segment
 * that now contains it (FindTrackSegment), then walks the intervening points and
 * adds (forward) or subtracts (backward) their segmentLength into
 * car->progressA (progress). The two mirror-image branches select forward vs
 * reverse lap direction from the direction flag g_RaceSeries. Register-pinned
 * locals (bv/ir) are load-bearing for the match.
 */
void AccumulateLapProgress(GameCarRuntime *car) {
    s32 r;
    s32 n;
    s32 i;
    s32 j;
    s32 fwd;
    s32 back;
    register s32 bv asm("$2");
    s32 count;
    GameTrackPoint *array;

    n = 1;
    r = FindTrackSegment(car, car->trackPointIndex);
    if (r < 0) {
        car->activeFlag = -1;
        return;
    }

    if (g_RaceSeries == 0) {
        if (r != car->trackPointIndex) {
            count = g_TrackPointCount;
            array = g_TrackPoints;
            do {
                j = car->trackPointIndex - n;
                back = j;
                if (j < 0) {
                    back = j + count;
                }
                fwd = (car->trackPointIndex + n) % count;
                if (r == back) {
                    s32 ir;
                    for (i = 0; i < n; i++) {
                        j = car->trackPointIndex - i;
                        ir = j;
                        if (j < 0) {
                            ir = j + count;
                        }
                        car->progressA += (s16)array[ir].segmentLength;
                    }
                    break;
                }
                if (r == fwd) {
                    for (i = 1; i <= n; i++) {
                        car->progressA -= (s16)array[(car->trackPointIndex + i) % count].segmentLength;
                    }
                    break;
                }
                n++;
            } while (r != car->trackPointIndex);
        }
    } else {
        if (r != car->trackPointIndex) {
            count = g_TrackPointCount;
            array = g_TrackPoints;
            do {
                j = car->trackPointIndex - n;
                back = j;
                if (j < 0) {
                    back = j + count;
                }
                fwd = (car->trackPointIndex + n) % count;
                bv = back;
                if (r == fwd) {
                    for (i = 0; i < n; i++) {
                        car->progressA += (s16)array[(car->trackPointIndex + i) % count].segmentLength;
                    }
                    break;
                }
                if (r == bv) {
                    register s32 ir asm("$3");
                    for (i = 1; i <= n; i++) {
                        j = car->trackPointIndex - i;
                        ir = j;
                        if (j < 0) {
                            ir = j + count;
                        }
                        car->progressA -= (s16)array[ir].segmentLength;
                    }
                    break;
                }
                n++;
            } while (r != car->trackPointIndex);
        }
    }
    car->trackPointIndex = r;
}
