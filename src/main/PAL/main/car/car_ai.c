#include "game/car.h"
#include "game/track.h"
#include "game/race.h"
/*
 * Put every car on the speed key it has already reached, at the start of a
 * race. The list is ordered along the track and ends with a -1.
 */
void SeedCarRouteMarkers(void) {
    s32 series = g_RaceSeries;
    s32 carIndex;

    for (carIndex = 0; carIndex < RACE_CAR_SLOT_COUNT; carIndex++) {
        s32 position = g_Cars[carIndex].trackProgress >> 4;
        s32 index;

        g_Cars[carIndex].routeMarkerActive = 1;
        for (index = 0; index < 0x30; index++) {
            s32 progress =
                g_TrackEventData->aiSpeedKeys[series][index].progress;

            if (progress == -1) {
                g_Cars[carIndex].routeMarkerIndex = 0;
                break;
            }
            if (position >= progress) {
                g_Cars[carIndex].routeMarkerIndex = index;
                break;
            }
        }
    }
}

/*
 * The racing line the AI is following: a list of stretches of track, each
 * saying how far off the centre line a car may drift there. A car past the end
 * of its current stretch takes the next one, wrapping at the list's -1.
 *
 * The nudge only applies to the front four and only when nobody is alongside,
 * so cars in traffic keep whatever line the collision code left them on.
 */
void ApplyCarRacingLineHint(GameCarRuntime *car, s32 carIndex) {
    GameCarAiBlock *ai = GetCarAiBlock(car);
    s32 series = g_RaceSeries;
    s32 position = car->trackProgress >> 4;
    TrackRacingLineHint *hint;

    /* Before the first stretch of the lap, the list starts over. */
    if (position < 0x20) {
        car->routeIndex = 0;
        position = 0;
    }
    hint = &g_TrackEventData->racingLineHints[series][car->routeIndex];

    if (hint->end < position) {
        ai->routeIndex++;
        if (g_TrackEventData->racingLineHints[series][ai->routeIndex].start ==
            -1) {
            ai->routeIndex = 0;
        }
        ai->racingLineHintState = 0;
        return;
    }
    if (position < hint->start) {
        car->racingLineHintState = 0;
        return;
    }
    if (carIndex < 4 && car->nearbyCarCount == 0) {
        s32 offset = car->aiLateralOffset;

        if (hint->minHeight < offset && offset < hint->maxHeight) {
            car->aiLateralOffset = offset + hint->heightAdjustment;
        }
    }
}

/*
 * How hard an AI car is allowed to accelerate right now.
 *
 * The track carries a list of keys, each a position along the track and a
 * target speed for each of the front four grid slots. A car sitting between
 * two keys gets its limit interpolated between them; a car that has run off
 * either end of its current pair steps its marker towards where it actually
 * is and gets nothing this frame. Cars further back use fourth place's target,
 * tapered by grid slot.
 */
void UpdateCarAiTargetSpeed(GameCarRuntime *car, s32 carIndex) {
    TrackAiSpeedKey *lowKey;
    TrackAiSpeedKey *highKey;
    TrackAiSpeedKey *table;
    GameCarAiBlock *ai;
    s32 position;
    s32 marker;
    s32 lowProgress;
    s32 highProgress;
    s32 lowSpeed;
    s32 highSpeed;
    s32 pitch;

    position = car->trackProgress >> 4;
    marker = car->routeMarkerIndex;
    ai = GetCarAiBlock(car);
    if (position < 0x20 || marker < 0) {
        car->routeMarkerIndex = 0;
        marker = 0;
    }

    table = g_TrackEventData->aiSpeedKeys[g_RaceSeries];
    lowKey = &table[marker];
    highKey = &table[marker + 1];
    lowProgress = lowKey->progress;
    highProgress = highKey->progress;
    if (carIndex < 4) {
        lowSpeed = lowKey->slotTargetSpeeds[carIndex];
        highSpeed = highKey->slotTargetSpeeds[carIndex];
    } else {
        s32 taper = 0x55 - carIndex;
        lowSpeed = (lowKey->slotTargetSpeeds[3] * taper) / 100;
        highSpeed = (highKey->slotTargetSpeeds[3] * taper) / 100;
    }

    pitch = 0;
    if (position >= lowProgress && position <= highProgress) {
        s32 range = highProgress - lowProgress;
        s32 blended;

        pitch = lowKey->pitch;
        if (range <= 0) {
            range = 1;
        }
        blended = lowSpeed +
                  (((highSpeed - lowSpeed) * (position - lowProgress)) / range);
        ai->accelerationLimit = (((blended * 1168) / 160) * 6) / 100;
    } else {
        ai->markerDirection = 1;
        ai->markerCounter += (highProgress < position) ? 1 : -1;
        if (position < 0x20) {
            ai->markerCounter = 0;
        }
    }

    if (ai->markerDirection != 0) {
        UpdateCarSlideAngle(car, (s16)pitch);
    }
}
