#include "game/car.h"
#include "game/race.h"
#include "game/track.h"

/* Return the track crest crossed this frame, or zero when none was crossed. */
s32 GetCarCrestTrigger(GameCarRuntime *car) {
    s32 low;
    s32 high;
    s32 row;
    s32 i;
    TrackCrestEvent *events;

    if (car->speed < 0x320) return 0;

    high = car->trackProgress;
    low = car->previousTrackProgress;
    row = car->facingBackwards;
    events = g_TrackEventData->crestEvents[row];

    if (g_RaceSeries != 0) {
        high = g_TrackLength - high;
        low = g_TrackLength - low;
    }
    if (low >= high) {
        s32 swap = low;
        low = high;
        high = swap;
    }
    if (high - low >= 0x1000) {
        low = 0;
        high = 0;
    }

    for (i = 0; i < 8; i++) {
        if (events[i].motionValue == -1) return 0;
        if (low < events[i].progress && events[i].progress <= high)
            return events[i].motionValue;
    }
    return 0;
}

/* Start a crest hop, or update the body attitude while one is in progress. */
void UpdateCarCrestHop(GameCarRuntime *car) {
    s32 trigger;

    if (car->verticalMotionState != 0) {
        s32 curve = car->verticalMotionTimer * car->verticalMotionTimer / 6;

        if (car->verticalPitch >= 0x12C) curve >>= 8;
        car->verticalPitch += curve;
        car->verticalRoll = (u16)car->verticalRoll + curve / 4;
        car->bodyPitch = car->verticalPitch;
        car->bodyRoll = car->verticalRoll;
        return;
    }

    trigger = GetCarCrestTrigger(car);
    if (trigger == 0) return;

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
