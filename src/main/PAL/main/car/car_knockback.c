#include "game/angle.h"
#include "game/diagnostics.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/state.h"
#include "game/render.h"

#include <stdlib.h>

enum {
    LOW_SPEED_KNOCKBACK_THRESHOLD = 0x321,
    MAX_SPEED_SCALED_KNOCKBACK = 0x708,
    TRACK_BOUNDARY_KNOCKBACK_DURATION = 0x1E,
    CAR_COLLISION_KNOCKBACK_DURATION = 0x0F,
    FIXED_TRACK_KNOCKBACK_MODE = 2,
    CAR_COLLISION_KNOCKBACK_MODE = 4,
};

static s32 TrackOutwardAngle(const GameCarRuntime *car) {
    return (s16)(ANGLE_THREE_QUARTER_TURN - car->trackHeading.half.low);
}

/* The road-edge response points a quarter turn into the course from the
 * outward track normal. Which quarter turn is chosen depends on the side of
 * the centre line the car occupies. */
static s32 TrackBoundaryPushAngle(const GameCarRuntime *car) {
    s32 outward = TrackOutwardAngle(car);

    return (outward + (car->trackLateralOffset < 0 ? -ANGLE_QUARTER_TURN
                                                   : ANGLE_QUARTER_TURN)) &
           ANGLE_MASK;
}

static s32 TrackBoundaryPushStrength(const GameCarRuntime *car) {
    s32 outward = TrackOutwardAngle(car);
    s32 approach = GetAngleDistance(outward, car->bodyYaw);
    s32 sine = rsin(approach);
    s32 speed;

    if (car->speed < LOW_SPEED_KNOCKBACK_THRESHOLD) {
        return (sine * 50) / 4096 + 10;
    }

    speed = car->speed <= MAX_SPEED_SCALED_KNOCKBACK
        ? car->speed
        : MAX_SPEED_SCALED_KNOCKBACK;
    return (speed * sine) / 65536 + 10;
}

static void SetKnockbackVector(GameCarRuntime *car, s32 angle, s32 strength,
                               u16 duration) {
    car->motionActive = 1;
    car->motionTimer = duration;
    car->velocityX = (s16)((rsin(angle) * strength) / 4096);
    car->velocityZ = (s16)((rcos(angle) * strength) / 4096);
}

static void TraceCarKnockback(GameCarRuntime *car, s32 inputX, s32 inputZ,
                              s32 mode) {
    static int enabled = -1;
    static int exactTimer = -1;
    static int firstTimer = -1;
    static int lastTimer = -1;

    if (enabled < 0) {
        const char *exact = DiagnosticsValue("car.knockback_trace_timer");
        const char *first = DiagnosticsValue("car.knockback_trace_timer_min");
        const char *last = DiagnosticsValue("car.knockback_trace_timer_max");

        enabled = DiagnosticsEnabled("car.knockback_trace");
        exactTimer = exact != NULL ? (int)strtol(exact, NULL, 0) : -1;
        firstTimer = first != NULL ? (int)strtol(first, NULL, 0) : -1;
        lastTimer = last != NULL ? (int)strtol(last, NULL, 0) : -1;
    }

    if (enabled &&
        (exactTimer < 0 || g_SceneTimer == exactTimer) &&
        (firstTimer < 0 || g_SceneTimer >= firstTimer) &&
        (lastTimer < 0 || g_SceneTimer <= lastTimer)) {
        Trace("car-knockback", "timer=%d player=%d input=%d,%d mode=%d "
              "output=%d,%d duration=%d heading=%d lateral=%d", g_SceneTimer,
              car == AsRivalCar(&g_PlayerCar), inputX, inputZ, mode,
              car->velocityX, car->velocityZ, car->motionTimer,
              car->trackHeading.half.low, car->trackLateralOffset);
    }
}

void ApplyCarKnockback(GameCarRuntime *car) {
    if (!car->motionActive) {
        return;
    }

    if (car->motionTimer <= 1) {
        car->motionActive = 0;
        car->motionTimer = 0;
    } else {
        car->motionTimer--;
    }

    car->x -= car->velocityX;
    car->z -= car->velocityZ;
    car->velocityX = (s16)(car->velocityX * 7 / 8);
    car->velocityZ = (s16)(car->velocityZ * 7 / 8);
}

void SetCarKnockback(GameCarRuntime *car, s32 x, s32 z, s32 mode) {
    if (mode < FIXED_TRACK_KNOCKBACK_MODE) {
        SetKnockbackVector(car, TrackBoundaryPushAngle(car),
                           TrackBoundaryPushStrength(car),
                           TRACK_BOUNDARY_KNOCKBACK_DURATION);
    } else if (mode < CAR_COLLISION_KNOCKBACK_MODE) {
        SetKnockbackVector(car, TrackBoundaryPushAngle(car), 20,
                           CAR_COLLISION_KNOCKBACK_DURATION);
    } else {
        car->motionActive = 1;
        car->motionTimer = CAR_COLLISION_KNOCKBACK_DURATION;
        car->velocityX = (s16)(x / 2);
        car->velocityZ = (s16)(z / 2);
    }

    TraceCarKnockback(car, x, z, mode);
}
