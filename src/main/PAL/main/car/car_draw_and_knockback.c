#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/random.h"
void DrawCars(void) {
    GameCarRuntime *base;
    s32 i;

    base = g_Cars;
    SelectModelBank(1);

    for (i = 0; i < 11; i++) {
        if (base->activeFlag != -1) {
            if (base->aiEnabled == 1) {
                DrawCar(GetCarRenderObject(base));
            }
        }
        base++;
    }
}

void DrawPlayerCarOnly(void) {
    SelectModelBank(1);
    DrawCar(GetCarRenderObject(g_Cars));
}

/*
 * A knock to the body. Strength one is the straight drop a landing gives, and
 * takes its size from how long the car was in the air. Anything else leans the
 * body along the track: how far the car is turned away from the road under it,
 * scaled by how fast it is going, and thrown to one side or the other at
 * random. Strengths other than one and two do nothing.
 */
void StartCarBodyKick(s32 strength, GameCarRuntime *car) {
    s32 lean;
    s32 speedOverWalking;

    car->motionMode = strength;
    if (strength == 1) {
        car->motionModeTimer = 0x1E;
        car->motionValue.value = car->verticalMotionTimer << 3;
        return;
    }
    if (strength != 2) {
        return;
    }

    /* The blend weight was missing here, so the angle came out mixed by
     * whatever happened to sit in the second argument slot. Every other
     * caller passes the car's own position between the two points. */
    lean = GetAngleDistance(
        InterpolateTrackAngle(car->trackPointIndex, car->segmentFraction),
        car->bodyYaw);
    if (lean >= 0x401) {
        lean = 0x800 - lean;
    }

    speedOverWalking = car->speed - 0x140;
    if (speedOverWalking < 0) {
        car->motionValue.value = 0;
    } else {
        car->motionValue.value = (speedOverWalking * lean) / 4096;
    }
    car->motionModeTimer = 0x1E;
    if (Random15() & 0x80) {
        car->motionValue.value = -car->motionValue.unsignedValue;
    }
}
