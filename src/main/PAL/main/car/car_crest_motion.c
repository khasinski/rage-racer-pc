#include "game/car.h"
#include "game/car_motion_internal.h"
#include "game/integer.h"
#include "game/race.h"
#include "game/track.h"

enum {
    CREST_MINIMUM_SPEED = 0x320,
    MAX_CREST_PROGRESS_STEP = 0x1000,
    CREST_ATTITUDE_CURVE_DIVISOR = 6,
    CREST_STEEP_PITCH = 0x12C,
    CREST_STEEP_CURVE_DIVISOR = 256,
    CREST_ROLL_CURVE_DIVISOR = 4,
    CREST_SPEED_RATE_DIVISOR = -4800,
};

/* Return the track crest crossed this frame, or zero when none was crossed. */
s32 GetCarCrestTrigger(const GameCarRuntime *car) {
    s32 low;
    s32 high;
    s32 row;
    s32 i;
    const TrackCrestEvent *events;

    if (car->speed < CREST_MINIMUM_SPEED || g_TrackEventData == NULL ||
        g_TrackLength <= 0) {
        return 0;
    }

    high = car->trackProgress;
    low = car->previousTrackProgress;
    row = car->facingBackwards != 0;
    events = g_TrackEventData->crestEvents[row];

    if (g_RaceSeries != 0) {
        high = WrapSigned32((int64_t)g_TrackLength - high);
        low = WrapSigned32((int64_t)g_TrackLength - low);
    }
    if (low >= high) {
        s32 swap = low;
        low = high;
        high = swap;
    }
    if (WrapSigned32((int64_t)high - low) >= MAX_CREST_PROGRESS_STEP) {
        low = 0;
        high = 0;
    }

    for (i = 0; i < TRACK_CREST_EVENT_COUNT; i++) {
        if (events[i].motionValue == -1) {
            return 0;
        }
        if (low < events[i].progress && events[i].progress <= high) {
            return events[i].motionValue;
        }
    }
    return 0;
}

/* Start a crest hop, or update the body attitude while one is in progress. */
void UpdateCarCrestHop(GameCarRuntime *car) {
    s32 trigger;

    if (car->verticalMotionState != CAR_VERTICAL_GROUNDED) {
        s32 curve = car->verticalMotionTimer * car->verticalMotionTimer /
                    CREST_ATTITUDE_CURVE_DIVISOR;

        if (car->verticalPitch >= CREST_STEEP_PITCH) {
            curve /= CREST_STEEP_CURVE_DIVISOR;
        }
        car->verticalPitch = WrapSigned16(
            (s32)car->verticalPitch + curve);
        car->verticalRoll = WrapSigned16(
            (s32)car->verticalRoll + curve / CREST_ROLL_CURVE_DIVISOR);
        car->bodyPitch = car->verticalPitch;
        car->bodyRoll = car->verticalRoll;
        return;
    }

    trigger = GetCarCrestTrigger(car);
    if (trigger == 0) {
        return;
    }

    car->verticalMotionState = CAR_VERTICAL_RISING;
    if (trigger > 0) {
        car->verticalMotionRate = WrapSigned16(
            WrapSigned32((int64_t)trigger * car->speed) /
            CREST_SPEED_RATE_DIVISOR);
    } else {
        car->verticalMotionState = CAR_VERTICAL_AT_CREST;
        car->verticalMotionRate = WrapSigned16(-(int64_t)trigger);
    }
    car->verticalMotionTimer = 0;
    car->verticalPitch = WrapSigned16(car->bodyPitch);
    car->verticalRoll = WrapSigned16(car->bodyRoll);
    car->verticalTargetY = WrapSigned16(car->y);
}
