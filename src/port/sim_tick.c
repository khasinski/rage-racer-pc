#include "rage/sim_tick.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/angle.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"
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

        if (field->kind == SIM_FIELD_COUNTDOWN_S16) {
            if (next > previous) {
                continue;
            }
            delta = (int64_t)next - previous;
            if (delta > INT32_MAX) delta = INT32_MAX;
            if (delta < INT32_MIN) delta = INT32_MIN;
            WriteField(now, field,
                       (int32_t)((int64_t)previous +
                                 SimHalfDelta((int32_t)delta, parity ^ 1u)));
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
    {offsetof(GameCarRuntime, targetYaw), SIM_FIELD_ANGLE32},
    {offsetof(GameCarRuntime, baseBodyYaw), SIM_FIELD_ANGLE32},
    {offsetof(GameCarRuntime, worldVelocityX), SIM_FIELD_S32},
    {offsetof(GameCarRuntime, worldVelocityZ), SIM_FIELD_S32},
    {offsetof(GameCarRuntime, yawRate), SIM_FIELD_S32},
    {offsetof(GameCarRuntime, slideInput), SIM_FIELD_S32},
    {offsetof(GameCarRuntime, engineRpm), SIM_FIELD_S32},
    {offsetof(GameCarRuntime, aiLateralOffset), SIM_FIELD_S16},
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
    {offsetof(PlayerCarRuntime, drive.trackCurveBias), SIM_FIELD_S16},
    {offsetof(PlayerCarRuntime, drive.coastFrames), SIM_FIELD_S32},
    {offsetof(PlayerCarRuntime, drive.launchEnergy), SIM_FIELD_S32},
    {offsetof(PlayerCarRuntime, drive.steeringLoadAngle), SIM_FIELD_S32},
    {offsetof(PlayerCarRuntime, drive.spinRate), SIM_FIELD_S32},
    {offsetof(PlayerCarRuntime, drive.launchSpeed), SIM_FIELD_S32},
    {offsetof(PlayerCarRuntime, drive.yawOffset), SIM_FIELD_S32},
    {offsetof(PlayerCarRuntime, drive.engineRpm), SIM_FIELD_S32},
    {offsetof(PlayerCarRuntime, drive.targetHeading), SIM_FIELD_ANGLE32},
    {offsetof(PlayerCarRuntime, drive.drivetrainTorque), SIM_FIELD_S32},
};

/* Counters the player's step ticks down or up once per call. */
typedef struct CounterSnapshot {
    s32 driveBoostTimer;
    s32 autoShiftCooldown;
    s32 engineRpm;
    s32 standingStartSpin;
    s32 shiftTargetRpm;
    s32 shiftSoundLevel;
    s16 gripLossTimer;
    s16 steerHoldFrames;
} CounterSnapshot;

static CounterSnapshot TakeCounters(void) {
    CounterSnapshot snapshot;

    snapshot.driveBoostTimer = g_DriveBoostTimer;
    snapshot.autoShiftCooldown = g_AutoShiftCooldown;
    snapshot.engineRpm = g_EngineRpm;
    snapshot.standingStartSpin = g_StandingStartSpin;
    snapshot.shiftTargetRpm = g_ShiftTargetRpm;
    snapshot.shiftSoundLevel = g_ShiftSoundLevel;
    snapshot.gripLossTimer = g_GripLossTimer;
    snapshot.steerHoldFrames = g_SteerHoldFrames;
    return snapshot;
}

static void BlendCounters(const CounterSnapshot *before) {
    unsigned parity = s_parity;

    if (g_DriveBoostTimer <= before->driveBoostTimer) {
        g_DriveBoostTimer = before->driveBoostTimer +
            SimHalfDelta(g_DriveBoostTimer - before->driveBoostTimer,
                         parity ^ 1u);
    }
    if (g_AutoShiftCooldown <= before->autoShiftCooldown &&
        !(g_AutoShiftCooldown == 0 && before->autoShiftCooldown > 2)) {
        g_AutoShiftCooldown = before->autoShiftCooldown +
            SimHalfDelta(g_AutoShiftCooldown - before->autoShiftCooldown,
                         parity ^ 1u);
    }
    g_EngineRpm = before->engineRpm +
        SimHalfDelta(g_EngineRpm - before->engineRpm, parity);
    g_EngineRpmSnapshot = g_EngineRpm;
    g_StandingStartSpin = before->standingStartSpin +
        SimHalfDelta(g_StandingStartSpin - before->standingStartSpin, parity);
    if (g_GripLossTimer <= before->gripLossTimer &&
        !(g_GripLossTimer == 0 && before->gripLossTimer > 1)) {
        g_GripLossTimer = (s16)(before->gripLossTimer +
            SimHalfDelta(g_GripLossTimer - before->gripLossTimer,
                         parity ^ 1u));
    }
    if (g_SteerHoldFrames > before->steerHoldFrames) {
        g_SteerHoldFrames = (s16)(before->steerHoldFrames +
            SimHalfDelta(g_SteerHoldFrames - before->steerHoldFrames, parity));
    }
}

static void SimBlendCarSubsystems(GameCarRuntime *after,
                                  const GameCarRuntime *before,
                                  s32 unblendedY, s32 unblendedBodyPitch,
                                  s32 unblendedBodyRoll,
                                  s32 unblendedWheelRotation,
                                  unsigned parity) {
    /* Wheel rotation: step forward in [0, ANGLE_FULL_TURN] even when the
     * per-frame step exceeds ANGLE_HALF_TURN (speeds 683..1365), and keep
     * CAR_WHEEL_BLUR_FLAG (0x1000) decoupled from the 12-bit angle wrap. */
    {
        int32_t wheelDelta = (int32_t)(((uint32_t)unblendedWheelRotation -
                                        (uint32_t)before->wheelRotation) &
                                       (uint32_t)ANGLE_MASK);
        int32_t wheelAngle = ((int32_t)before->wheelRotation +
                              SimHalfDelta(wheelDelta, parity)) &
                             ANGLE_MASK;
        after->wheelRotation =
            wheelAngle | (unblendedWheelRotation & CAR_WHEEL_BLUR_FLAG);
    }

    /* Knockback: decay velocity and countdown timer at retail rate while
     * preserving newly triggered knockback impulses intact. Hold the timer on
     * substep 0 and decrement on substep 1 so both half-steps of the final
     * active frame execute. */
    if (before->motionActive != 0 &&
        (int32_t)before->motionTimer - (int32_t)after->motionTimer == 1 &&
        after->velocityX == (s16)(before->velocityX * 7 / 8) &&
        after->velocityZ == (s16)(before->velocityZ * 7 / 8)) {
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
        } else {
            after->bodyKickOffset = 0;
        }
    }

    /* Crest hop / airborne jump arc: preserve initial pitch/roll seed on
     * takeoff, and step the airborne parabola tick at retail rate. */
    if (before->verticalMotionState == CAR_VERTICAL_GROUNDED &&
        after->verticalMotionState != CAR_VERTICAL_GROUNDED) {
        after->y = unblendedY;
        after->verticalPitch = (s16)unblendedBodyPitch;
        after->verticalRoll = (s16)unblendedBodyRoll;
    } else if (before->verticalMotionState != CAR_VERTICAL_GROUNDED &&
               after->verticalMotionState != CAR_VERTICAL_GROUNDED) {
        int32_t tickDelta =
            (int32_t)after->verticalMotionTimer -
            (int32_t)before->verticalMotionTimer;
        after->verticalMotionTimer = (s16)(before->verticalMotionTimer +
            SimHalfDelta(tickDelta, parity));
        if (after->verticalMotionTimer != before->verticalMotionTimer ||
            after->verticalMotionState != before->verticalMotionState) {
            after->y = unblendedY;
        }
        after->bodyPitch = after->verticalPitch;
        after->bodyRoll = after->verticalRoll;
    } else if (before->verticalMotionState != CAR_VERTICAL_GROUNDED &&
               after->verticalMotionState == CAR_VERTICAL_GROUNDED) {
        after->y = unblendedY;
        after->bodyPitch = unblendedBodyPitch;
        after->bodyRoll = unblendedBodyRoll;
        after->verticalPitch = 0;
        after->verticalRoll = 0;
    }
}

void SimUpdatePlayerCar(PlayerCarRuntime *car) {
    enum {
        STANDING_START_SPIN_THRESHOLD = 11,
        AIRBORNE_DECAY_NUMERATOR = 31,
        AIRBORNE_DECAY_DENOMINATOR = 32,
        BODY_LIFT_DECAY_NUMERATOR = 2,
        BODY_LIFT_DECAY_DENOMINATOR = 3,
    };
    PlayerCarRuntime before;
    CounterSnapshot counters;
    s32 rivalSpeedBefore[RACE_CAR_SLOT_COUNT];
    s32 rivalAccelBefore[RACE_CAR_SLOT_COUNT];
    s32 unblendedY;
    s32 unblendedBodyPitch;
    s32 unblendedBodyRoll;
    s32 unblendedWheelRotation;
    s32 unblendedHeadingAngle;
    s32 unblendedSteerPos;
    s32 unblendedSteeringAngle;
    s32 unblendedCoastFrames;
    s32 unblendedLaunchEnergy;
    s32 unblendedSpinRate;
    s32 unblendedLaunchSpeed;
    s32 unblendedYawOffset;
    s32 unblendedDrivetrainTorque;
    s32 unblendedEngineRpm;
    s16 unblendedEngineLoad;
    s16 unblendedClutch;
    s16 unblendedJumpTimer;
    int index;

    if (!s_scaled) {
        UpdatePlayerCar(car);
        return;
    }
    before = *car;
    counters = TakeCounters();
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        rivalSpeedBefore[index] = g_Cars[index].speed;
        rivalAccelBefore[index] = g_Cars[index].acceleration;
    }
    UpdatePlayerCar(car);
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        if (g_Cars[index].activeFlag != -1 &&
            g_Cars[index].collisionFlag != 0) {
            g_Cars[index].speed = rivalSpeedBefore[index] +
                SimHalfDelta(g_Cars[index].speed - rivalSpeedBefore[index],
                             s_parity);
            g_Cars[index].acceleration = rivalAccelBefore[index] +
                SimHalfDelta(g_Cars[index].acceleration -
                                 rivalAccelBefore[index],
                             s_parity);
        }
    }
    unblendedY = car->y;
    unblendedBodyPitch = car->bodyPitch;
    unblendedBodyRoll = car->bodyRoll;
    unblendedWheelRotation = car->wheelRotation;
    unblendedHeadingAngle = car->headingAngle;
    unblendedSteerPos = car->drive.steerPos;
    unblendedSteeringAngle = car->steeringAngle;
    unblendedCoastFrames = car->drive.coastFrames;
    unblendedLaunchEnergy = car->drive.launchEnergy;
    unblendedSpinRate = car->drive.spinRate;
    unblendedLaunchSpeed = car->drive.launchSpeed;
    unblendedYawOffset = car->drive.yawOffset;
    unblendedDrivetrainTorque = car->drive.drivetrainTorque;
    unblendedEngineRpm = car->drive.engineRpm;
    unblendedEngineLoad = car->drive.engineLoad;
    unblendedClutch = car->drive.clutch;
    unblendedJumpTimer = car->drive.jumpTimer;
    SimBlendFields(car, &before, kPlayerFields,
                   sizeof(kPlayerFields) / sizeof(kPlayerFields[0]),
                   s_parity);
    if (before.drive.motionState == CAR_MOTION_TAKEOFF) {
        car->drive.targetHeading = car->bodyYaw;
    }
    if ((before.drive.motionState == CAR_MOTION_DRIVING &&
         before.drive.gearDisp == before.drive.gear &&
         before.drive.clutch > 1 &&
         car->drive.clutch < before.drive.clutch) ||
        (before.drive.motionState == CAR_MOTION_AIRBORNE &&
         before.drive.jumpTimer > 1 &&
         car->drive.jumpTimer < before.drive.jumpTimer)) {
        car->drive.engineRpm = unblendedEngineRpm;
    }
    if (before.drive.steerPos != 0 && unblendedSteerPos == 0 &&
        (car->drive.trackCurveMode != 0 || g_RacePhase < RACE_PHASE_ACTIVE)) {
        car->drive.steerPos = 0;
        car->steeringAngle = unblendedSteeringAngle;
    }
    if (unblendedCoastFrames == 0) {
        car->drive.coastFrames = 0;
    }
    if (before.drive.motionState != CAR_MOTION_TAKEOFF &&
        car->drive.motionState == CAR_MOTION_TAKEOFF) {
        car->drive.launchEnergy = unblendedLaunchEnergy;
        car->drive.spinRate = unblendedSpinRate;
        car->drive.bodyLiftOffset = 0;
        car->drive.coastFrames = 0;
    }
    if (car->drive.clutch > before.drive.clutch) {
        car->drive.engineLoad = unblendedEngineLoad;
    }
    if (car->drive.jumpTimer > before.drive.jumpTimer) {
        car->drive.engineLoad = unblendedEngineLoad;
        car->drive.drivetrainTorque = unblendedDrivetrainTorque;
        car->drive.launchSpeed = unblendedLaunchSpeed;
        car->drive.yawOffset = unblendedYawOffset;
        car->drive.spinRate = unblendedSpinRate;
        if (before.drive.motionState == CAR_MOTION_TAKEOFF) {
            car->headingAngle = unblendedHeadingAngle;
        }
    }
    if (unblendedClutch == 0 &&
        (car->drive.motionState == CAR_MOTION_TAKEOFF ||
         car->drive.motionState == CAR_MOTION_STANDING_START ||
         (car->speed == 0 && car->drive.gear < before.drive.gear))) {
        car->drive.clutch = 0;
    }
    if (unblendedJumpTimer == 0 &&
        (car->drive.motionState == CAR_MOTION_TAKEOFF ||
         car->drive.motionState == CAR_MOTION_STANDING_START)) {
        car->drive.jumpTimer = 0;
    }
    /* Clutch engagement and airborne drivetrain reconnect: when SimBlendFields
     * holds the countdown at 1 on substep 0, keep the pre-step state active so
     * the second half-step of the final frame runs before reconnecting; when
     * the countdown reaches 0 on substep 1, commit the zeroed load. */
    if (before.drive.clutch > 0 &&
        car->drive.drivetrainCoupled != before.drive.drivetrainCoupled) {
        if (car->drive.clutch > 0) {
            car->drive.drivetrainCoupled = before.drive.drivetrainCoupled;
            car->drive.engineLoad = before.drive.engineLoad;
        } else {
            car->drive.engineLoad = 0;
        }
    }
    if (before.drive.jumpTimer > 0 &&
        before.drive.motionState == CAR_MOTION_AIRBORNE &&
        car->drive.motionState == CAR_MOTION_DRIVING) {
        if (car->drive.jumpTimer > 0) {
            s32 decayedYawOffset =
                (s32)(((int64_t)before.drive.yawOffset *
                       AIRBORNE_DECAY_NUMERATOR) /
                      AIRBORNE_DECAY_DENOMINATOR);
            s32 decayedLaunchSpeed =
                (s32)(((int64_t)before.drive.launchSpeed *
                       AIRBORNE_DECAY_NUMERATOR) /
                      AIRBORNE_DECAY_DENOMINATOR);
            s16 decayedBodyLift = (s16)(before.drive.bodyLiftOffset *
                                        BODY_LIFT_DECAY_NUMERATOR /
                                        BODY_LIFT_DECAY_DENOMINATOR);

            car->drive.motionState = CAR_MOTION_AIRBORNE;
            car->drive.shiftRpmDelta = before.drive.shiftRpmDelta;
            g_ShiftSoundLevel = counters.shiftSoundLevel;
            car->drive.yawOffset = before.drive.yawOffset +
                SimHalfDelta(decayedYawOffset - before.drive.yawOffset,
                             s_parity);
            car->drive.launchSpeed = before.drive.launchSpeed +
                SimHalfDelta(decayedLaunchSpeed - before.drive.launchSpeed,
                             s_parity);
            car->drive.bodyLiftOffset = (s16)(before.drive.bodyLiftOffset +
                SimHalfDelta(decayedBodyLift - before.drive.bodyLiftOffset,
                             s_parity));
        } else {
            car->drive.yawOffset = 0;
            car->drive.launchSpeed = 0;
            car->drive.bodyLiftOffset = 0;
            car->drive.shiftRpmDelta = 0;
        }
    }
    SimBlendCarSubsystems(AsRivalCar(car),
                          (const GameCarRuntime *)&before, unblendedY,
                          unblendedBodyPitch, unblendedBodyRoll,
                          unblendedWheelRotation, s_parity);
    BlendCounters(&counters);
    if (g_ShiftTargetRpm < counters.shiftTargetRpm &&
        car->drive.jumpTimer <= before.drive.jumpTimer &&
        before.drive.gearDisp == before.drive.gear) {
        g_ShiftTargetRpm = counters.shiftTargetRpm +
            SimHalfDelta(g_ShiftTargetRpm - counters.shiftTargetRpm, s_parity);
    }
    if (before.drive.motionState == CAR_MOTION_STANDING_START &&
        counters.standingStartSpin >= STANDING_START_SPIN_THRESHOLD) {
        if (s_parity == 0 ||
            g_StandingStartSpin >= STANDING_START_SPIN_THRESHOLD) {
            car->drive.motionState = CAR_MOTION_STANDING_START;
            if (s_parity == 0 &&
                g_StandingStartSpin < STANDING_START_SPIN_THRESHOLD) {
                g_StandingStartSpin = STANDING_START_SPIN_THRESHOLD;
            }
        }
    }
    if (car->drive.motionState != CAR_MOTION_STANDING_START) {
        car->drive.standingStartBounceX = 0;
        car->drive.standingStartBounceY = 0;
    }
}

void SimUpdateRaceCars(void) {
    GameCarRuntime before[RACE_CAR_SLOT_COUNT];
    s16 beforeCooldowns[RIVAL_CONTENDER_COUNT];
    int index;

    if (!s_scaled) {
        UpdateRaceCars();
        return;
    }
    memcpy(before, g_Cars, sizeof(before));
    memcpy(beforeCooldowns, g_RivalCueCooldowns, sizeof(beforeCooldowns));
    UpdateRaceCars();
    for (index = 0; index < RIVAL_CONTENDER_COUNT; index++) {
        if (g_RivalCueCooldowns[index] > beforeCooldowns[index]) {
            g_RivalCueCooldowns[index] = (s16)(beforeCooldowns[index] +
                SimHalfDelta(g_RivalCueCooldowns[index] -
                                 beforeCooldowns[index],
                             s_parity));
        }
    }
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        s32 unblendedY;
        s32 unblendedBodyPitch;
        s32 unblendedBodyRoll;
        s32 unblendedWheelRotation;
        s32 unblendedSlideInput;

        if (before[index].activeFlag == -1) continue;
        unblendedY = g_Cars[index].y;
        unblendedBodyPitch = g_Cars[index].bodyPitch;
        unblendedBodyRoll = g_Cars[index].bodyRoll;
        unblendedWheelRotation = g_Cars[index].wheelRotation;
        unblendedSlideInput = g_Cars[index].slideInput;
        SimBlendFields(&g_Cars[index], &before[index], kRivalFields,
                       sizeof(kRivalFields) / sizeof(kRivalFields[0]),
                       s_parity);
        if (before[index].slideInput == 0 && before[index].yawRate == 0 &&
            unblendedSlideInput != 0) {
            g_Cars[index].slideInput = unblendedSlideInput;
        }
        if (before[index].slideActive != 0 &&
            g_Cars[index].slideActive == 0 &&
            g_Cars[index].yawRate != 0) {
            g_Cars[index].slideActive = before[index].slideActive;
        }
        SimBlendCarSubsystems(&g_Cars[index], &before[index], unblendedY,
                              unblendedBodyPitch, unblendedBodyRoll,
                              unblendedWheelRotation, s_parity);
    }
}

static const SimField kChaseFields[] = {
    {offsetof(Camera, chase.previousYaw), SIM_FIELD_ANGLE32},
    {offsetof(Camera, chase.rampNeg), SIM_FIELD_S32},
    {offsetof(Camera, chase.rampPos), SIM_FIELD_S32},
};

void SimUpdateCamera(Camera *camera, CameraViewMode mode, GameCarRuntime *car) {
    Camera before;
    s32 unblendedRampNeg;
    s32 unblendedRampPos;

    if (!s_scaled) {
        UpdateCamera(camera, mode, car);
        return;
    }
    before = *camera;
    UpdateCamera(camera, mode, car);
    if (before.previousMode != CAMERA_VIEW_CHASE &&
        camera->previousMode == CAMERA_VIEW_CHASE) {
        return;
    }
    unblendedRampNeg = camera->chase.rampNeg;
    unblendedRampPos = camera->chase.rampPos;
    SimBlendFields(camera, &before, kChaseFields,
                   sizeof(kChaseFields) / sizeof(kChaseFields[0]), s_parity);
    if (unblendedRampNeg == 0) {
        camera->chase.rampNeg = 0;
    }
    if (unblendedRampPos == 0) {
        camera->chase.rampPos = 0;
    }
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
