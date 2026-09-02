#include "game/car.h"
#include "game/track.h"
#include "game/race.h"
#include "psyq/gte.h"

/*
 * Jump / launch setup: when GetCarCrestTrigger reports a marker crossing, seeds the
 * launch trajectory and snapshots the car's render offsets (bodyPitch/bodyRoll
 * and y). verticalMotionState 1 is the rising jump phase.
 */

void UpdateCarBodyKick(GameCarRuntime *car) {
    s32 value;
    s32 wave;
    s32 amplitude;
    s32 timer;

    if (car->motionMode == 0) {
        return;
    }

    car->motionModeTimer--;
    if ((s16)car->motionModeTimer == 0) {
        car->motionMode = 0;
        car->bodyKickOffset = 0;
    }

    timer = car->motionModeTimer;
    amplitude = timer * car->motionValue.value / 128;
    wave = rsin(((timer * 3) << 12) / 30) * amplitude;
    value = wave / 2048;

    switch (car->motionMode) {
    case 1:
    case 5:
        car->bodyKickOffset = value + amplitude;
        car->bodyPitch += car->bodyKickOffset;
        car->bodyKickOffset = value + amplitude / 2;
        car->bodyRoll += car->bodyKickOffset / 2;
        break;

    case 2:
        if (car->verticalMotionState != 0) {
            break;
        }
        car->bodyRoll += value;
        break;

    case 3:
        car->bodyKickOffset = value + amplitude;
        car->bodyPitch += car->bodyKickOffset;
        break;

    case 4:
        car->bodyRoll += value;
        break;
    }
}

/*
 * The crest the car drove over this frame, if any: the number the track's own
 * table gives that crest, or 0. Each row of the table is up to eight crests
 * for one direction of travel, ordered along the track and ended by a -1.
 */
s32 GetCarCrestTrigger(GameCarRuntime *car) {
    s32 low;
    s32 high;
    s32 row;
    s32 i;
    TrackCrestEvent *events;

    if (car->speed < 0x320) {
        return 0;
    }

    high = car->trackProgress;
    low = car->previousTrackProgress;
    row = car->facingBackwards;
    events = g_TrackEventData->crestEvents[row];

    if (g_RaceSeries != 0) {
        high = g_TrackLength - high;
        low = g_TrackLength - low;
    }

    /* The stretch covered this frame, low end first. */
    if (low >= high) {
        s32 swap = low;
        low = high;
        high = swap;
    }
    /* A gap that big is the lap wrapping round, not a stretch of track. */
    if (high - low >= 0x1000) {
        low = 0;
        high = 0;
    }

    for (i = 0; i < 8; i++) {
        if (events[i].motionValue == -1) {
            return 0;
        }
        if (low < events[i].progress && events[i].progress <= high) {
            return events[i].motionValue;
        }
    }
    return 0;
}

void UpdateCarCrestHop(GameCarRuntime *car) {
    s32 trigger;

    if (car->verticalMotionState != 0) {
        s32 curve = car->verticalMotionTimer * car->verticalMotionTimer / 6;

        if (car->verticalPitch >= 0x12C) {
            curve >>= 8;
        }
        car->verticalPitch += curve;
        car->verticalRoll = (u16)car->verticalRoll + curve / 4;
        car->bodyPitch = car->verticalPitch;
        car->bodyRoll = car->verticalRoll;
        return;
    }

    trigger = GetCarCrestTrigger(car);
    if (trigger == 0) {
        return;
    }

    car->verticalMotionState = 1;
    if (trigger > 0) {
        car->verticalMotionRate = trigger * car->speed / -4800;
    } else {
        car->verticalMotionState = 2;
        car->verticalMotionRate = -trigger;
    }

    car->verticalMotionTimer = 0;
    car->verticalPitch = (u16)car->bodyPitch;
    car->verticalRoll = (u16)car->bodyRoll;
    car->verticalTargetY = (u16)car->y;
}

/* Ease a slide back towards straight: 15/16 of the yaw rate each frame,
 * rounding towards zero, and drop the marker once it reaches nothing. */
static void SettleSlide(GameCarAiBlock *ai) {
    s32 rate = ai->yawRate;

    if (rate == 0) {
        return;
    }
    rate = rate * 15 * 2;
    rate /= 32;
    ai->yawRate = rate;
    if (rate == 0) {
        ai->markerDirection = 0;
    }
}

/*
 * The AI cars' slide: a rival that is not sliding and not turning gets nudged
 * into one in proportion to its speed and its place in the field, and a car
 * that is sliding turns that input into yaw rate, capped either way.
 */
void UpdateCarSlideAngle(GameCarRuntime *car, s32 carIndex) {
    GameCarRuntime *obj = car;
    GameCarAiBlock *ai;
    s32 adjusted;
    s32 input;
    s32 rate;

    ai = GetCarAiBlock(obj);
    if (obj->slideInput.value == 0) {
        if (obj->yawRate == 0 && carIndex != 0) {
            /* Start a slide, away from the racing line in reverse races. */
            if (obj->speed < 0x3C1) {
                return;
            }
            input = (carIndex * obj->speed) / 0x320;
            obj->slideInput.value = g_RaceSeries != 0 ? -input : input;
            obj->yawRate = 0;
            return;
        }
        if (ai->slideInput.value == 0) {
            SettleSlide(ai);
            return;
        }
    }

    /* Decay the input by 31/32, rounding towards zero, and take half of what
     * is left off the yaw rate. The half carries a rounding bit of its own:
     * the sign of the product before the shift, which is set only while that
     * product is still negative after the correction. */
    adjusted = ai->slideInput.value * 31;
    if (adjusted < 0) {
        adjusted += 0x1F;
    }
    input = adjusted >> 5;
    ai->slideInput.value = input;
    rate = ai->yawRate - ((input + (s32)((u32)adjusted >> 31)) >> 1);
    ai->yawRate = rate;
    if (rate >= 0x2BC) {
        ai->yawRate = 0x2BC;
    } else if (rate < -0x2BB) {
        ai->yawRate = -0x2BC;
    }
}

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
 * target speed per gear. A car sitting between two keys gets its limit
 * interpolated between them; a car that has run off either end of its current
 * pair steps its marker towards where it actually is and gets nothing this
 * frame. Above fourth the target is fourth's, scaled down as the gear climbs.
 */
void UpdateCarAiTargetSpeed(GameCarRuntime *car, s32 gear) {
    TrackAiSpeedKey *keys[2];
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
    /*
     * Read before the resets below: retail still indexes with the old marker
     * for this frame, and only the next frame sees the zero.
     *
     * routeMarkerIndex and the AI view's markerCounter are the same halfword,
     * signed here and unsigned there. The signed read is what makes the `< 0`
     * test below mean anything, so both spellings have to stay.
     */
    marker = car->routeMarkerIndex;
    ai = GetCarAiBlock(car);
    if (position < 0x20 || marker < 0) {
        car->routeMarkerIndex = 0;
    }

    table = g_TrackEventData->aiSpeedKeys[g_RaceSeries];
    keys[0] = &table[marker];
    keys[1] = &table[marker + 1];
    lowProgress = keys[0]->progress;
    highProgress = keys[1]->progress;
    if (gear < 4) {
        lowSpeed = keys[0]->targetSpeeds[gear];
        highSpeed = keys[1]->targetSpeeds[gear];
    } else {
        s32 taper = 0x55 - gear;
        lowSpeed = (keys[0]->targetSpeeds[3] * taper) / 100;
        highSpeed = (keys[1]->targetSpeeds[3] * taper) / 100;
    }

    pitch = 0;
    if (position >= lowProgress && position <= highProgress) {
        s32 range = highProgress - lowProgress;
        s32 blended;

        pitch = keys[0]->pitch;
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
