#include "game/car.h"
#include "game/car_motion_internal.h"

enum { RIVAL_TRACK_INSET = 0x3C };

/*
 * This stays two passes: every active car updates its lap progress before any
 * car applies knockback and resamples its track-relative pose.
 */
void PlaceRivalCarsOnTrack(void) {
    CarTrackLimits limits;
    s32 index;

    limits.rightInset = RIVAL_TRACK_INSET;
    limits.leftInset = -RIVAL_TRACK_INSET;
    limits.rightContact = CAR_TRACK_CONTACT_NONE;
    limits.leftContact = CAR_TRACK_CONTACT_NONE;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        if (g_Cars[index].activeFlag != -1) {
            AccumulateLapProgress(&g_Cars[index]);
        }
    }
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];

        if (car->activeFlag == -1) {
            continue;
        }
        if (car->motionActive) {
            ApplyCarKnockback(car);
        }
        UpdateCarTrackState(car, car->trackPointIndex, &limits);
    }
}
