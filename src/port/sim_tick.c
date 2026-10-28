#include "rage/sim_tick.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/angle.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"
#include "game/track.h"
#include "runtime_config.h"

enum {
    RETAIL_MULTIPLIER = 1,
    DOUBLE_MULTIPLIER = 2,
    /* One video field in the units VSync(1) counts. */
    FIELD_SYNC_UNITS = 0x80,
};

static int s_multiplier;
static int s_multiplierKnown;
static int s_scaled;
static int s_substep;
static unsigned s_parity;
static int s_running;
static unsigned short s_carriedPressed;
static unsigned s_traceTick;

int32_t SimHalfDelta(int32_t delta, unsigned parity) {
    int64_t value = (int64_t)delta + (int64_t)(parity & 1u);

    /* Floor division, so +1 and -1 both sum back to themselves over a pair. */
    return (int32_t)(value >= 0 ? value / 2 : -((-value + 1) / 2));
}

static int32_t ReadField(const unsigned char *base, const SimField *field) {
    if (field->kind == SIM_FIELD_S16 ||
        field->kind == SIM_FIELD_COUNTDOWN_S16) {
        int16_t value;
        memcpy(&value, base + field->offset, sizeof(value));
        return value;
    } else {
        int32_t value;
        memcpy(&value, base + field->offset, sizeof(value));
        return value;
    }
}

static void WriteField(unsigned char *base, const SimField *field,
                       int32_t value) {
    if (field->kind == SIM_FIELD_S16 ||
        field->kind == SIM_FIELD_COUNTDOWN_S16) {
        int16_t narrow = (int16_t)value;
        memcpy(base + field->offset, &narrow, sizeof(narrow));
    } else {
        memcpy(base + field->offset, &value, sizeof(value));
    }
}

void SimBlendFields(void *current, const void *before, const SimField *fields,
                    size_t count, unsigned parity) {
    unsigned char *now = current;
    const unsigned char *then = before;
    size_t index;

    for (index = 0; index < count; index++) {
        const SimField *field = &fields[index];
        int32_t previous = ReadField(then, field);
        int32_t next = ReadField(now, field);
        int64_t delta;

        if (field->kind == SIM_FIELD_COUNTDOWN_S16 && next > previous) {
            continue;
        }
        if (field->kind == SIM_FIELD_ANGLE32) {
            int32_t wrapped = (int32_t)((uint32_t)(next - previous) &
                                        (uint32_t)ANGLE_MASK);

            if (wrapped >= ANGLE_HALF_TURN) wrapped -= ANGLE_FULL_TURN;
            delta = wrapped;
        } else {
            delta = (int64_t)next - previous;
        }
        if (delta > INT32_MAX) delta = INT32_MAX;
        if (delta < INT32_MIN) delta = INT32_MIN;
        WriteField(now, field,
                   (int32_t)((int64_t)previous +
                             SimHalfDelta((int32_t)delta, parity)));
    }
}

int SimTickMultiplier(void) {
    if (!s_multiplierKnown) {
        const char *value = RuntimeConfigGet("game.tick_rate");

        s_multiplierKnown = 1;
        s_multiplier = RETAIL_MULTIPLIER;
        if (value == NULL || value[0] == '\0' ||
            strcmp(value, "retail") == 0) {
            /* Retail cadence. */
        } else if (strcmp(value, "double") == 0) {
            s_multiplier = DOUBLE_MULTIPLIER;
        } else {
            fprintf(stderr,
                    "rage-port: ignoring game.tick_rate=%s "
                    "(expected retail or double)\n", value);
        }
        if (s_multiplier == DOUBLE_MULTIPLIER) {
            fprintf(stderr,
                    "rage-port: experimental double-rate race simulation\n");
        }
    }
    return s_multiplier;
}

void SimTickSetMultiplier(int multiplier) {
    s_multiplierKnown = 1;
    s_multiplier = multiplier >= DOUBLE_MULTIPLIER ? DOUBLE_MULTIPLIER
                                                   : RETAIL_MULTIPLIER;
    s_running = 0;
    s_scaled = 0;
    s_substep = 0;
    s_parity = 0;
    s_carriedPressed = 0;
}

void SimRaceFrameBegin(int live) {
    if (SimTickMultiplier() != DOUBLE_MULTIPLIER || !live) {
        s_scaled = 0;
        s_substep = 0;
        s_running = 0;
        s_parity = 0;
        return;
    }
    /* The first live frame always opens a retail pair. */
    s_substep = s_running ? !s_substep : 0;
    s_parity = (unsigned)s_substep;
    s_running = 1;
    s_scaled = 1;
}

int SimRaceScaled(void) { return s_scaled; }
int SimRaceSubstep(void) { return s_substep; }
unsigned SimRaceParity(void) { return s_parity; }

int SimRaceFrameSyncThreshold(int retailThreshold) {
    return s_scaled ? FIELD_SYNC_UNITS : retailThreshold;
}

void SimRaceCarryPadEdges(unsigned short *pressed) {
    if (!s_scaled) {
        s_carriedPressed = 0;
        return;
    }
    if (s_substep) {
        s_carriedPressed = (unsigned short)(s_carriedPressed | *pressed);
        *pressed = 0;
    } else {
        *pressed = (unsigned short)(*pressed | s_carriedPressed);
        s_carriedPressed = 0;
    }
}

/* Fields that move continuously, where half a step is half the change. The
 * rest keep the value the retail step produced: enumerations, latches, indices
 * derived from the position, values rebuilt from scratch each step, and lap
 * progress, which grows in whole-segment jumps when the car crosses a track
 * point. Halving a jump would lose the other half for good, so a car would
 * never complete a lap. */
#define CAR_FIELDS(T)                                                    \
    {offsetof(T, x), SIM_FIELD_S32},                                     \
    {offsetof(T, y), SIM_FIELD_S32},                                     \
    {offsetof(T, z), SIM_FIELD_S32},                                     \
    {offsetof(T, motionX), SIM_FIELD_S32},                               \
    {offsetof(T, motionY), SIM_FIELD_S32},                               \
    {offsetof(T, motionZ), SIM_FIELD_S32},                               \
    {offsetof(T, bodyPitch), SIM_FIELD_ANGLE32},                         \
    {offsetof(T, bodyYaw), SIM_FIELD_ANGLE32},                           \
    {offsetof(T, bodyRoll), SIM_FIELD_ANGLE32},                          \
    {offsetof(T, steeringAngle), SIM_FIELD_S32},                         \
    {offsetof(T, wheelRotation), SIM_FIELD_ANGLE32},                     \
    {offsetof(T, modelPitch), SIM_FIELD_ANGLE32},                        \
    {offsetof(T, modelYaw), SIM_FIELD_ANGLE32},                          \
    {offsetof(T, modelRoll), SIM_FIELD_ANGLE32},                         \
    {offsetof(T, modelY), SIM_FIELD_S32},                                \
    {offsetof(T, bodyRollVelocity), SIM_FIELD_S32},                      \
    {offsetof(T, tiltCounter), SIM_FIELD_S16},                           \
    {offsetof(T, verticalPitch), SIM_FIELD_S16},                         \
    {offsetof(T, bodyKickOffset), SIM_FIELD_S16},                        \
    {offsetof(T, verticalRoll), SIM_FIELD_S16},                          \
    {offsetof(T, headingAngle), SIM_FIELD_ANGLE32},                      \
    {offsetof(T, speed), SIM_FIELD_S32},                                 \
    {offsetof(T, acceleration), SIM_FIELD_S32}

static const SimField kRivalFields[] = {
    CAR_FIELDS(GameCarRuntime),
    {offsetof(GameCarRuntime, worldVelocityX), SIM_FIELD_S32},
    {offsetof(GameCarRuntime, worldVelocityZ), SIM_FIELD_S32},
    {offsetof(GameCarRuntime, yawRate), SIM_FIELD_S32},
    {offsetof(GameCarRuntime, slideInput), SIM_FIELD_S32},
    {offsetof(GameCarRuntime, engineRpm), SIM_FIELD_S32},
    {offsetof(GameCarRuntime, boostTimer), SIM_FIELD_COUNTDOWN_S16},
};

static const SimField kPlayerFields[] = {
    CAR_FIELDS(PlayerCarRuntime),
    {offsetof(PlayerCarRuntime, drive.accelPos), SIM_FIELD_S32},
    {offsetof(PlayerCarRuntime, drive.brakePos), SIM_FIELD_S32},
    {offsetof(PlayerCarRuntime, drive.steerPos), SIM_FIELD_S32},
    {offsetof(PlayerCarRuntime, drive.engineLoad), SIM_FIELD_S16},
    {offsetof(PlayerCarRuntime, drive.steeringGrip), SIM_FIELD_S16},
    {offsetof(PlayerCarRuntime, drive.clutch), SIM_FIELD_COUNTDOWN_S16},
    {offsetof(PlayerCarRuntime, drive.jumpTimer), SIM_FIELD_COUNTDOWN_S16},
    {offsetof(PlayerCarRuntime, drive.bodyLiftOffset), SIM_FIELD_S16},
    {offsetof(PlayerCarRuntime, drive.coastFrames), SIM_FIELD_S32},
    {offsetof(PlayerCarRuntime, drive.launchEnergy), SIM_FIELD_S32},
    {offsetof(PlayerCarRuntime, drive.steeringLoadAngle), SIM_FIELD_S32},
    {offsetof(PlayerCarRuntime, drive.spinRate), SIM_FIELD_S32},
    {offsetof(PlayerCarRuntime, drive.launchSpeed), SIM_FIELD_S32},
    {offsetof(PlayerCarRuntime, drive.yawOffset), SIM_FIELD_S32},
    {offsetof(PlayerCarRuntime, drive.standingStartBounceX), SIM_FIELD_S32},
    {offsetof(PlayerCarRuntime, drive.standingStartBounceY), SIM_FIELD_S32},
    {offsetof(PlayerCarRuntime, drive.engineRpm), SIM_FIELD_S32},
    {offsetof(PlayerCarRuntime, drive.drivetrainTorque), SIM_FIELD_S32},
};

/* Counters the player's step ticks down or up once per call. */
typedef struct CounterSnapshot {
    s32 driveBoostTimer;
    s32 autoShiftCooldown;
    s32 engineRpm;
    s32 standingStartSpin;
    s16 gripLossTimer;
    s16 steerHoldFrames;
} CounterSnapshot;

static CounterSnapshot TakeCounters(void) {
    CounterSnapshot snapshot;

    snapshot.driveBoostTimer = g_DriveBoostTimer;
    snapshot.autoShiftCooldown = g_AutoShiftCooldown;
    snapshot.engineRpm = g_EngineRpm;
    snapshot.standingStartSpin = g_StandingStartSpin;
    snapshot.gripLossTimer = g_GripLossTimer;
    snapshot.steerHoldFrames = g_SteerHoldFrames;
    return snapshot;
}

static void BlendCounters(const CounterSnapshot *before) {
    unsigned parity = s_parity;

    if (g_DriveBoostTimer <= before->driveBoostTimer) {
        g_DriveBoostTimer = before->driveBoostTimer +
            SimHalfDelta(g_DriveBoostTimer - before->driveBoostTimer, parity);
    }
    if (g_AutoShiftCooldown <= before->autoShiftCooldown) {
        g_AutoShiftCooldown = before->autoShiftCooldown +
            SimHalfDelta(g_AutoShiftCooldown - before->autoShiftCooldown, parity);
    }
    g_EngineRpm = before->engineRpm +
        SimHalfDelta(g_EngineRpm - before->engineRpm, parity);
    g_StandingStartSpin = before->standingStartSpin +
        SimHalfDelta(g_StandingStartSpin - before->standingStartSpin, parity);
    if (g_GripLossTimer <= before->gripLossTimer) {
        g_GripLossTimer = (s16)(before->gripLossTimer +
            SimHalfDelta(g_GripLossTimer - before->gripLossTimer, parity));
    }
    g_SteerHoldFrames = (s16)(before->steerHoldFrames +
        SimHalfDelta(g_SteerHoldFrames - before->steerHoldFrames, parity));
}

static void SimBlendCarSubsystems(GameCarRuntime *after,
                                  const GameCarRuntime *before,
                                  s32 unblendedY, s32 unblendedBodyPitch,
                                  s32 unblendedBodyRoll, unsigned parity) {
    /* Knockback: decay velocity and countdown timer at retail rate while
     * preserving newly triggered knockback impulses intact. Hold the timer on
     * substep 0 and decrement on substep 1 so both half-steps of the final
     * active frame execute. */
    if (before->motionActive != 0 &&
        after->motionTimer < before->motionTimer) {
        int32_t timerDelta =
            (int32_t)after->motionTimer - (int32_t)before->motionTimer;
        after->motionTimer = (u16)((int32_t)before->motionTimer +
                                   SimHalfDelta(timerDelta, parity ^ 1u));
        after->motionActive = after->motionTimer > 0 ? 1 : 0;
        after->velocityX = (s16)(before->velocityX +
            SimHalfDelta(after->velocityX - before->velocityX, parity));
        after->velocityZ = (s16)(before->velocityZ +
            SimHalfDelta(after->velocityZ - before->velocityZ, parity));
    }

    /* Body kick: step countdown timer at retail rate (hold on substep 0,
     * decrement on substep 1) and keep motionMode active until zero. */
    if (before->motionMode != CAR_BODY_KICK_INACTIVE &&
        after->motionModeTimer < before->motionModeTimer) {
        after->motionModeTimer = (s16)(before->motionModeTimer +
            SimHalfDelta(after->motionModeTimer - before->motionModeTimer,
                         parity ^ 1u));
        if (after->motionModeTimer > 0) {
            after->motionMode = before->motionMode;
        }
    }

    /* Crest hop / airborne jump arc: preserve initial pitch/roll seed on
     * takeoff, and step the airborne parabola tick at retail rate. */
    if (before->verticalMotionState == CAR_VERTICAL_GROUNDED &&
        after->verticalMotionState != CAR_VERTICAL_GROUNDED) {
        after->verticalPitch = (s16)unblendedBodyPitch;
        after->verticalRoll = (s16)unblendedBodyRoll;
    } else if (before->verticalMotionState != CAR_VERTICAL_GROUNDED &&
               after->verticalMotionState != CAR_VERTICAL_GROUNDED) {
        int32_t tickDelta =
            (int32_t)after->verticalMotionTimer -
            (int32_t)before->verticalMotionTimer;
        after->verticalMotionTimer = (s16)(before->verticalMotionTimer +
            SimHalfDelta(tickDelta, parity));
        if (parity & 1u) {
            after->y = unblendedY;
        }
        after->bodyPitch = after->verticalPitch;
        after->bodyRoll = after->verticalRoll;
    }
}

void SimUpdatePlayerCar(PlayerCarRuntime *car) {
    PlayerCarRuntime before;
    CounterSnapshot counters;
    s32 unblendedY;
    s32 unblendedBodyPitch;
    s32 unblendedBodyRoll;

    if (!s_scaled) {
        UpdatePlayerCar(car);
        return;
    }
    before = *car;
    counters = TakeCounters();
    UpdatePlayerCar(car);
    unblendedY = car->y;
    unblendedBodyPitch = car->bodyPitch;
    unblendedBodyRoll = car->bodyRoll;
    SimBlendFields(car, &before, kPlayerFields,
                   sizeof(kPlayerFields) / sizeof(kPlayerFields[0]),
                   s_parity);
    SimBlendCarSubsystems(AsRivalCar(car),
                          (const GameCarRuntime *)&before, unblendedY,
                          unblendedBodyPitch, unblendedBodyRoll, s_parity);
    BlendCounters(&counters);
}

void SimUpdateRaceCars(void) {
    GameCarRuntime before[RACE_CAR_SLOT_COUNT];
    int index;

    if (!s_scaled) {
        UpdateRaceCars();
        return;
    }
    memcpy(before, g_Cars, sizeof(before));
    UpdateRaceCars();
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        s32 unblendedY;
        s32 unblendedBodyPitch;
        s32 unblendedBodyRoll;

        if (before[index].activeFlag == -1) continue;
        unblendedY = g_Cars[index].y;
        unblendedBodyPitch = g_Cars[index].bodyPitch;
        unblendedBodyRoll = g_Cars[index].bodyRoll;
        SimBlendFields(&g_Cars[index], &before[index], kRivalFields,
                       sizeof(kRivalFields) / sizeof(kRivalFields[0]),
                       s_parity);
        SimBlendCarSubsystems(&g_Cars[index], &before[index], unblendedY,
                              unblendedBodyPitch, unblendedBodyRoll, s_parity);
    }
}

static const SimField kChaseFields[] = {
    {offsetof(Camera, chase.previousYaw), SIM_FIELD_ANGLE32},
    {offsetof(Camera, chase.rampNeg), SIM_FIELD_S32},
    {offsetof(Camera, chase.rampPos), SIM_FIELD_S32},
};

void SimUpdateCamera(Camera *camera, CameraViewMode mode, GameCarRuntime *car) {
    Camera before;

    if (!s_scaled) {
        UpdateCamera(camera, mode, car);
        return;
    }
    before = *camera;
    UpdateCamera(camera, mode, car);
    SimBlendFields(camera, &before, kChaseFields,
                   sizeof(kChaseFields) / sizeof(kChaseFields[0]), s_parity);
}

void SimRaceTrace(void) {
    static int enabled = -1;
    const PlayerCarRuntime *car = &g_PlayerCar;

    if (enabled < 0) enabled = RuntimeConfigEnabled("diagnostics.race_trace");
    if (!enabled || g_RacePhase != RACE_PHASE_ACTIVE) {
        if (g_RacePhase < RACE_PHASE_ACTIVE) s_traceTick = 0;
        return;
    }
    fprintf(stderr,
            "racetrace frame=%d tick=%u mult=%d x=%d z=%d speed=%d rpm=%d "
            "gear=%d heading=%d yaw=%d progress=%d rival0=%d,%d\n",
            g_FrameCounter, s_traceTick++,
            s_scaled ? DOUBLE_MULTIPLIER : RETAIL_MULTIPLIER,
            car->x, car->z, car->speed, car->drive.engineRpm, car->drive.gear,
            car->headingAngle & ANGLE_MASK, car->bodyYaw & ANGLE_MASK,
            car->trackProgress, g_Cars[0].x, g_Cars[0].z);
}
