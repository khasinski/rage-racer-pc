#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/state.h"
#include "game/track.h"
#include "rage/trace.h"


/* Every car starts the frame from the heading it settled on last frame, and
 * keeps only the low bit of whatever it was touching. */
static void StartCarFrames(void) {
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];

        car->bodyYaw = car->baseBodyYaw;
        car->collisionFlag &= 1;
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
        if (index >= RIVAL_CONTENDER_COUNT &&
            (index & 1) != (g_AnimTimer & 1)) {
            continue;
        }
        if (g_Cars[index].activeFlag != -1) {
            UpdateCarTrafficAvoidance(&g_Cars[index], index);
        }
    }
}

/* Each car against the ones behind it, so every pair is tested once. */
static void CollideAllCars(void) {
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT - 1; index++) {
        CollideRivalCars(index);
    }
}

/* Where each car wants to be on the road, and how it gets there. */
static void SteerAllCars(void) {
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];

        if (car->activeFlag == -1) {
            continue;
        }
        UpdateCarAiTargetSpeed(car, index);
        ApplyCarRacingLineHint(car, index);
        ClampCarLateralOffset(car, index);
        SteerCarAlongRoute(car);
    }
}

/* Everyone ahead of the player is held back a little, nearest one first. */
static void SlowTheCarsAhead(void) {
    s32 rank = g_ClosestRivalRank;

    if (rank >= RIVAL_CONTENDER_COUNT) {
        rank = RIVAL_CONTENDER_COUNT - 1;
    }
    while (rank > 0) {
        SlowRivalAhead(rank);
        rank--;
    }
}

/* Run the rival-car passes for an interactive race. Rear rivals split their
 * traffic-avoidance work across alternating frames. */
void UpdateRaceCars(void) {
    StartCarFrames();
    RankContenders();
    AvoidTrafficThisFrame();
    CollideAllCars();
    SteerAllCars();
    UpdateRivalRubberBand();
    SlowTheCarsAhead();
    AccelerateRaceRivals();
    MoveRivalCars();
    PlaceRivalCarsOnTrack();
    UpdateRivalBodyMotion();
}

/* Runs the corresponding all-cars pass for attract and replay scenes. */
void UpdateAttractCars(void) {
    s32 i;

    TraceCarStates();
    for (i = 0; i < RACE_CAR_SLOT_COUNT; i++) {
        GameCarRuntime *car = &g_Cars[i];

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
    AccelerateAttractRivals();
    MoveRivalCars();
    PlaceRivalCarsOnTrack();
    UpdateRivalBodyMotion();
}
