#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/state.h"
#include "game/track.h"
#include "rage/trace.h"


/*
 * Runs the rival-car update passes used by an interactive race. Cars 4..10
 * split their traffic-avoidance work across alternating frames.
 */
/* Every car starts the frame from the heading it settled on last frame, and
 * keeps only the low bit of whatever it was touching. */
static void StartCarFrames(void) {
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];

        car->reservedF8 = 0;
        car->bodyYaw = car->baseBodyYaw;
        car->collisionFlag = (u16)car->collisionFlag & 1;
    }
}

/*
 * Looking for a way past the car in front. Only the leading four do it every
 * frame; the rest take turns, odd cars on odd frames, which halves the work
 * without anyone noticing at the back of the field.
 */
static void AvoidTrafficThisFrame(void) {
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        if (index >= 4 && (index & 1) != (g_AnimTimer & 1)) {
            continue;
        }
        if (g_Cars[index].activeFlag != -1) {
            UpdateCarTrafficAvoidance(&g_Cars[index], index);
        }
    }
}

/*
 * How hard each car pulls this frame, and how far it swings towards where it
 * wants to be pointing. A car on a boost gets the boost's own acceleration
 * until it is already quick enough, and its own otherwise; the speed keeps a
 * little under two thirds of itself each frame, so the acceleration is what
 * holds it up.
 */
static void AccelerateAllCars(void) {
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];
        GameCarAiBlock *ai = GetCarAiBlock(car);

        if (car->activeFlag != -1) {
            if (car->boostTimer > 0) {
                if (car->boostAccelerationThreshold < car->boostTimer &&
                    car->speed >= 0x321) {
                    car->acceleration = 0;
                } else if (ai->accelerationLimit >= car->acceleration) {
                    car->acceleration = ai->boostAcceleration + car->acceleration;
                } else {
                    car->acceleration = ai->accelerationLimit;
                }
                ai->boostTimer--;
            } else if (car->accelerationLimit >= car->acceleration) {
                car->acceleration = car->accelerationStep + car->acceleration;
            } else {
                car->acceleration = car->accelerationLimit;
            }
            car->speed = car->speed * 0x5E / 100;
            car->speed = car->speed + car->acceleration;
            car->bodyYaw =
                GetAngleDelta(car->bodyYaw, ai->targetYaw) / 5 + car->bodyYaw;
        }
    }
}

/* Each car against the ones behind it, so every pair is tested once. */
static void CollideAllCars(void) {
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT - 1; index++) {
        CollideRivalCars(&g_Cars[index], index);
    }
}

/* Where each car wants to be on the road, and how it gets there. */
static void SteerAllCars(void) {
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];

        UpdateCarAiTargetSpeed(car, index);
        ApplyCarRacingLineHint(car, index);
        ClampCarLateralOffset(car, index);
        SteerCarAlongRoute(car);
    }
}

/*
 * How far round each car is, and where the track has put it. A rival's hull is
 * a fixed width here rather than measured from its corners, which is what the
 * player's car does.
 */
static void PlaceAllCarsOnTrack(void) {
    CarTrackLimits limits;
    s32 index;

    limits.rightInset = 0x3C;
    limits.leftInset = -0x3C;
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];

        if (car->activeFlag != -1) {
            AccumulateLapProgress(car);
        }
    }
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];

        if (car->activeFlag != -1) {
            if ((s16)car->motionTimer > 0) {
                ApplyCarKnockback(car);
            }
            UpdateCarTrackState(car, car->trackPointIndex, &limits);
        }
    }
}

/* Everyone ahead of the player is held back a little, nearest one first. */
static void SlowTheCarsAhead(void) {
    s16 rank = (s16)g_ClosestRivalRank;

    while ((s16)rank > 0) {
        s32 slot = (s16)rank;

        SlowRivalAhead(g_RankedCars[slot], slot);
        rank--;
    }
}

void UpdateRaceCars(void) {
    StartCarFrames();
    RankContenders();
    AvoidTrafficThisFrame();
    CollideAllCars();
    SteerAllCars();
    UpdateRivalRubberBand();
    SlowTheCarsAhead();
    AccelerateAllCars();
    MoveRivalCars();
    PlaceAllCarsOnTrack();
    UpdateRivalBodyMotion();
}

static void AccelerateAttractCars(void) {
    s32 i;

    for (i = 0; i < RACE_CAR_SLOT_COUNT; i++) {
        GameCarRuntime *car = &g_Cars[i];
        GameCarAiBlock *ai = GetCarAiBlock(car);

        if (car->activeFlag == -1) {
            continue;
        }
        if (car->acceleration < car->accelerationLimit) {
            car->acceleration += car->accelerationStep;
        } else {
            car->acceleration = car->accelerationLimit;
        }
        car->speed = car->speed * 94 / 100 + car->acceleration;
        car->bodyYaw += GetAngleDelta(car->bodyYaw, ai->targetYaw) / 5;
    }
}

/* Runs the corresponding all-cars pass for attract and replay scenes. */
void UpdateAttractCars(void) {
    s32 i;

    TraceCarStates();
    for (i = 0; i < RACE_CAR_SLOT_COUNT; i++) {
        GameCarRuntime *car = &g_Cars[i];

        car->reservedF8 = 0;
        car->collisionFlag = 0;
        car->bodyYaw = car->baseBodyYaw;
        if (g_TrackLength > 0) {
            car->progressA %= g_TrackLength;
        }
    }
    for (i = 0; i < RACE_CAR_SLOT_COUNT; i++) {
        if (g_Cars[i].activeFlag != -1) {
            UpdateCarTrafficAvoidance(&g_Cars[i], i);
        }
    }
    CollideAllCars();
    SteerAllCars();
    AccelerateAttractCars();
    MoveRivalCars();
    PlaceAllCarsOnTrack();
    UpdateRivalBodyMotion();
}
