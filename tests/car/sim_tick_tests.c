#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/angle.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"
#include "rage/sim_tick.h"

static int failures;

#define EXPECT_EQ(expected, actual) do {                                   \
    long long expectedValue = (long long)(expected);                       \
    long long actualValue = (long long)(actual);                           \
    if (expectedValue != actualValue) {                                    \
        fprintf(stderr, "%s:%d: expected %lld, got %lld\n", __FILE__,      \
                __LINE__, expectedValue, actualValue);                     \
        failures++;                                                        \
    }                                                                      \
} while (0)

#define EXPECT_NEAR(expected, actual, tolerance) do {                      \
    long long expectedValue = (long long)(expected);                       \
    long long actualValue = (long long)(actual);                           \
    long long difference = expectedValue - actualValue;                    \
    if (difference < 0) difference = -difference;                          \
    if (difference > (long long)(tolerance)) {                             \
        fprintf(stderr, "%s:%d: expected %lld +/- %lld, got %lld\n",       \
                __FILE__, __LINE__, expectedValue, (long long)(tolerance), \
                actualValue);                                              \
        failures++;                                                        \
    }                                                                      \
} while (0)

/* The game state the wrapper reads. */
PlayerCarRuntime g_PlayerCar;
GameCarRuntime g_Cars[RACE_CAR_SLOT_COUNT];
Camera g_Camera;
s32 g_DriveBoostTimer;
s32 g_AutoShiftCooldown;
s32 g_EngineRpm;
s32 g_EngineRpmSnapshot;
s32 g_StandingStartSpin;
s32 g_ShiftSoundLevel;
s32 g_ShiftTargetRpm;
s16 g_GripLossTimer;
s16 g_SteerHoldFrames;
s16 g_RivalCueCooldowns[RIVAL_CONTENDER_COUNT];
s16 g_RacePhase;
s32 g_FrameCounter;
static void (*s_CustomRivalStep)(void);

const char *RuntimeConfigGet(const char *key) {
    (void)key;
    return NULL;
}

int RuntimeConfigEnabled(const char *key) {
    (void)key;
    return 0;
}

/* A retail step with the shapes the real physics uses: a decay with a push,
 * an integration, a countdown and a wrapping angle. */
static void RetailStep(PlayerCarRuntime *car) {
    /* Lap progress grows in a single jump when a track point is crossed. */
    if (car->x >= 100 && car->trackPointIndex == 0) {
        car->trackPointIndex = 1;
        car->progressA += 300;
    }
    car->speed = car->speed * 94 / 100 + 50;
    car->x += car->speed;
    if (car->drive.motionState == CAR_MOTION_AIRBORNE) {
        s16 nextTimer = (s16)(car->drive.jumpTimer - 1);
        car->drive.jumpTimer = nextTimer < 0 ? 0 : nextTimer;
        if (car->drive.jumpTimer <= 0) {
            car->drive.motionState = CAR_MOTION_DRIVING;
            car->drive.shiftRpmDelta = 0;
        }
    } else {
        car->drive.jumpTimer--;
    }
    if (car->drive.clutch > 0) {
        car->drive.clutch--;
        if (car->drive.clutch <= 0) {
            car->drive.drivetrainCoupled = 1;
            car->drive.engineLoad = 0;
            car->drive.clutch = 0;
        }
    }
    car->drive.targetHeading = (car->drive.targetHeading + 12) & ANGLE_MASK;
    car->bodyYaw = (car->bodyYaw + 16) & ANGLE_MASK;
    car->drive.gear = 3;
    car->tiltCounter -= 4;
    if (car->motionActive) {
        if (car->motionTimer <= 1) {
            car->motionActive = 0;
            car->motionTimer = 0;
        } else {
            car->motionTimer--;
        }
        car->x -= car->velocityX;
        car->velocityX = (s16)(car->velocityX * 7 / 8);
    }
    if (car->motionMode != CAR_BODY_KICK_INACTIVE) {
        if (car->motionModeTimer <= 1) {
            car->motionMode = CAR_BODY_KICK_INACTIVE;
            car->motionModeTimer = 0;
        } else {
            car->motionModeTimer--;
        }
    }
    if (car->verticalMotionState != CAR_VERTICAL_GROUNDED) {
        s16 tick = (s16)(car->verticalMotionTimer + 1);
        car->verticalMotionTimer = tick;
        if (car->verticalMotionState == CAR_VERTICAL_AT_CREST) {
            car->y = car->verticalTargetY;
            if (car->x >= 100) {
                car->verticalMotionState = CAR_VERTICAL_FALLING;
                car->verticalMotionRate = tick;
            }
        } else if (car->verticalMotionState == CAR_VERTICAL_FALLING) {
            s16 fall = (s16)(tick - car->verticalMotionRate);
            car->y = car->verticalTargetY + fall * fall;
        } else {
            car->y = car->verticalTargetY + tick * tick;
        }
        car->verticalPitch = (s16)(car->verticalPitch + 6);
        car->bodyPitch = car->verticalPitch;
    }
    g_GripLossTimer--;
}

static void (*s_CustomPlayerStep)(PlayerCarRuntime *);
static void (*s_CustomCameraStep)(Camera *, CameraViewMode, GameCarRuntime *);

void UpdatePlayerCar(PlayerCarRuntime *car) {
    if (s_CustomPlayerStep != NULL) {
        s_CustomPlayerStep(car);
        return;
    }
    RetailStep(car);
}

void UpdateRaceCars(void) {
    int index;

    if (s_CustomRivalStep != NULL) {
        s_CustomRivalStep();
        return;
    }
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        if (g_Cars[index].activeFlag == -1) continue;
        g_Cars[index].speed = g_Cars[index].speed * 94 / 100 + 50;
        g_Cars[index].x += g_Cars[index].speed;
    }
}

void UpdateCamera(Camera *camera, CameraViewMode mode, GameCarRuntime *car) {
    if (s_CustomCameraStep != NULL) {
        s_CustomCameraStep(camera, mode, car);
        return;
    }
    (void)mode;
    (void)car;
    camera->chase.rampPos += 8;
    camera->chase.previousYaw = (camera->chase.previousYaw + 20) & ANGLE_MASK;
    camera->chase.damping = 7;
}

static void BeginLiveFrame(void) { SimRaceFrameBegin(1); }

static void TestHalfDelta(void) {
    EXPECT_EQ(0, SimHalfDelta(0, 0));
    EXPECT_EQ(0, SimHalfDelta(0, 1));
    EXPECT_EQ(5, SimHalfDelta(10, 0));
    EXPECT_EQ(5, SimHalfDelta(10, 1));
    /* An odd change alternates so two ticks add up to the full step. */
    EXPECT_EQ(1, SimHalfDelta(1, 0) + SimHalfDelta(1, 1));
    EXPECT_EQ(-1, SimHalfDelta(-1, 0) + SimHalfDelta(-1, 1));
    EXPECT_EQ(7, SimHalfDelta(7, 0) + SimHalfDelta(7, 1));
    EXPECT_EQ(-7, SimHalfDelta(-7, 0) + SimHalfDelta(-7, 1));
    EXPECT_EQ(INT32_MAX / 2, SimHalfDelta(INT32_MAX - 1, 0));
}

static void TestBlendFields(void) {
    typedef struct Sample {
        int32_t value;
        int16_t small;
        int16_t countdown;
        int32_t angle;
        int32_t untouched;
    } Sample;
    static const SimField fields[] = {
        {offsetof(Sample, value), SIM_FIELD_S32},
        {offsetof(Sample, small), SIM_FIELD_S16},
        {offsetof(Sample, countdown), SIM_FIELD_COUNTDOWN_S16},
        {offsetof(Sample, angle), SIM_FIELD_ANGLE32},
    };
    Sample before = {100, 10, 0, 4090, 5};
    Sample after = {120, 4, 10, 10, 9};

    SimBlendFields(&after, &before, fields, 4, 0);
    EXPECT_EQ(110, after.value);
    EXPECT_EQ(7, after.small);
    /* A newly loaded countdown keeps its full initial value. */
    EXPECT_EQ(10, after.countdown);
    /* 4090 -> 10 is a step of +16 across the wrap; half of it is +8. */
    EXPECT_EQ(4098, after.angle);
    EXPECT_EQ(9, after.untouched);

    after.angle = 4080;
    before.angle = 4090;
    before.countdown = 10;
    after.countdown = 8;
    SimBlendFields(&after, &before, fields, 4, 0);
    EXPECT_EQ(4085, after.angle);
    EXPECT_EQ(9, after.countdown);
}

static void TestFrameSequence(void) {
    SimTickSetMultiplier(1);
    BeginLiveFrame();
    EXPECT_EQ(0, SimRaceScaled());
    EXPECT_EQ(0, SimRaceSubstep());
    EXPECT_EQ(0x180, SimRaceFrameSyncThreshold(0x180));

    SimTickSetMultiplier(2);
    BeginLiveFrame();
    EXPECT_EQ(1, SimRaceScaled());
    EXPECT_EQ(0, SimRaceSubstep());
    EXPECT_EQ(0x80, SimRaceFrameSyncThreshold(0x180));
    BeginLiveFrame();
    EXPECT_EQ(1, SimRaceSubstep());
    EXPECT_EQ(1, SimRaceParity());
    BeginLiveFrame();
    EXPECT_EQ(0, SimRaceSubstep());
    BeginLiveFrame();
    EXPECT_EQ(1, SimRaceSubstep());

    /* Leaving the live phase falls back to the retail cadence, and the next
     * live run opens a fresh pair. */
    SimRaceFrameBegin(0);
    EXPECT_EQ(0, SimRaceScaled());
    EXPECT_EQ(0, SimRaceSubstep());
    EXPECT_EQ(0x180, SimRaceFrameSyncThreshold(0x180));
    BeginLiveFrame();
    EXPECT_EQ(0, SimRaceSubstep());
}

static void TestPadEdges(void) {
    unsigned short pressed;

    SimTickSetMultiplier(2);
    BeginLiveFrame();
    pressed = 0x0010;
    SimRaceCarryPadEdges(&pressed);
    EXPECT_EQ(0x0010, pressed);

    BeginLiveFrame(); /* substepwitholds edge until next full step */
    pressed = 0x0800;
    SimRaceCarryPadEdges(&pressed);
    EXPECT_EQ(0, pressed);

    BeginLiveFrame(); /* next full step receives the edge from the substep */
    pressed = 0x0001;
    SimRaceCarryPadEdges(&pressed);
    EXPECT_EQ(0x0801, pressed);

    BeginLiveFrame();
    BeginLiveFrame();
    pressed = 0;
    SimRaceCarryPadEdges(&pressed);
    EXPECT_EQ(0, pressed);
}

static void TestRetailPassThrough(void) {
    PlayerCarRuntime reference;

    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    SimTickSetMultiplier(1);
    BeginLiveFrame();
    reference = g_PlayerCar;
    RetailStep(&reference);
    SimUpdatePlayerCar(&g_PlayerCar);
    EXPECT_EQ(reference.speed, g_PlayerCar.speed);
    EXPECT_EQ(reference.x, g_PlayerCar.x);
    EXPECT_EQ(reference.bodyYaw, g_PlayerCar.bodyYaw);
}

/* Two half steps per retail step track the retail trajectory and share its
 * steady state. */
static void TestDoubleTracksRetail(void) {
    PlayerCarRuntime retail;
    int step;

    memset(&retail, 0, sizeof(retail));
    retail.drive.jumpTimer = 400;
    retail.motionActive = 1;
    retail.motionTimer = 15;
    retail.velocityX = 64;
    retail.motionMode = CAR_BODY_KICK_CORNERING;
    retail.motionModeTimer = 16;
    retail.verticalMotionState = CAR_VERTICAL_RISING;
    retail.verticalTargetY = 100;
    for (step = 0; step < 60; step++) RetailStep(&retail);

    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    g_PlayerCar.drive.jumpTimer = 400;
    g_PlayerCar.motionActive = 1;
    g_PlayerCar.motionTimer = 15;
    g_PlayerCar.velocityX = 64;
    g_PlayerCar.motionMode = CAR_BODY_KICK_CORNERING;
    g_PlayerCar.motionModeTimer = 16;
    g_PlayerCar.verticalMotionState = CAR_VERTICAL_RISING;
    g_PlayerCar.verticalTargetY = 100;
    g_GripLossTimer = 400;
    SimTickSetMultiplier(2);
    for (step = 0; step < 120; step++) {
        BeginLiveFrame();
        SimUpdatePlayerCar(&g_PlayerCar);
        if (step == 28) {
            /* Mid-last-frame of the 15-frame knockback: still active on substep 0. */
            EXPECT_EQ(1, g_PlayerCar.motionActive);
            EXPECT_EQ(1, g_PlayerCar.motionTimer);
        }
        if (step == 30) {
            /* Mid-last-frame of the 16-frame body kick: still active on substep 0. */
            EXPECT_EQ(CAR_BODY_KICK_CORNERING, g_PlayerCar.motionMode);
            EXPECT_EQ(1, g_PlayerCar.motionModeTimer);
        }
    }
    /* Same time, same place, within a percent. */
    EXPECT_NEAR(retail.x, g_PlayerCar.x, retail.x / 100);
    EXPECT_NEAR(retail.speed, g_PlayerCar.speed, 8);
    /* A countdown covers the same number of retail steps. */
    EXPECT_EQ(retail.drive.jumpTimer, g_PlayerCar.drive.jumpTimer);
    EXPECT_EQ(400 - 60, g_GripLossTimer);
    EXPECT_EQ(0, g_PlayerCar.motionActive);
    EXPECT_EQ(0, g_PlayerCar.motionTimer);
    EXPECT_EQ(CAR_BODY_KICK_INACTIVE, g_PlayerCar.motionMode);
    EXPECT_EQ(0, g_PlayerCar.motionModeTimer);
    EXPECT_EQ(retail.tiltCounter, g_PlayerCar.tiltCounter);
    EXPECT_EQ(retail.verticalMotionTimer, g_PlayerCar.verticalMotionTimer);
    EXPECT_EQ(retail.y, g_PlayerCar.y);
    EXPECT_EQ(retail.verticalPitch, g_PlayerCar.verticalPitch);
    /* The angle advanced by the same amount, and values the retail step
     * assigns outright keep that value. */
    EXPECT_EQ(retail.bodyYaw, g_PlayerCar.bodyYaw & ANGLE_MASK);
    EXPECT_EQ(3, g_PlayerCar.drive.gear);
    /* A one-off jump is not halved: it would never be made up. */
    EXPECT_EQ(300, retail.progressA);
    EXPECT_EQ(300, g_PlayerCar.progressA);

    for (step = 0; step < 4000; step++) RetailStep(&retail);
    for (step = 0; step < 8000; step++) {
        BeginLiveFrame();
        SimUpdatePlayerCar(&g_PlayerCar);
    }
    EXPECT_NEAR(retail.speed, g_PlayerCar.speed, 3);
}

static void TestRivalsAndCamera(void) {
    int step;

    memset(g_Cars, 0, sizeof(g_Cars));
    memset(&g_Camera, 0, sizeof(g_Camera));
    g_Cars[0].activeFlag = 1;
    g_Cars[1].activeFlag = -1;
    g_Cars[1].speed = 999;
    SimTickSetMultiplier(2);
    for (step = 0; step < 40; step++) {
        BeginLiveFrame();
        SimUpdateRaceCars();
        SimUpdateCamera(&g_Camera, CAMERA_VIEW_CHASE, &g_Cars[0]);
    }
    /* Twenty retail steps of the stub: the same speed and ramp growth. */
    {
        s32 speed = 0;
        int retail;

        for (retail = 0; retail < 20; retail++) speed = speed * 94 / 100 + 50;
        EXPECT_NEAR(speed, g_Cars[0].speed, 6);
    }
    EXPECT_EQ(999, g_Cars[1].speed);
    EXPECT_EQ(8 * 20, g_Camera.chase.rampPos);
    EXPECT_EQ((20 * 20) & ANGLE_MASK, g_Camera.chase.previousYaw);
    /* Unlisted camera state keeps what the retail update wrote. */
    EXPECT_EQ(7, g_Camera.chase.damping);
}

static void TestAirborneDrivetrainAndCrestFall(void) {
    PlayerCarRuntime retail;
    int step;

    memset(&retail, 0, sizeof(retail));
    retail.drive.motionState = CAR_MOTION_AIRBORNE;
    retail.drive.jumpTimer = 20;
    retail.drive.shiftRpmDelta = 400;
    retail.drive.clutch = 10;
    retail.drive.drivetrainCoupled = 0;
    retail.drive.engineLoad = 123;
    retail.verticalMotionState = CAR_VERTICAL_AT_CREST;
    retail.verticalMotionRate = 10;
    retail.verticalTargetY = 1346;
    for (step = 0; step < 25; step++) RetailStep(&retail);

    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    g_PlayerCar.drive.motionState = CAR_MOTION_AIRBORNE;
    g_PlayerCar.drive.jumpTimer = 20;
    g_PlayerCar.drive.shiftRpmDelta = 400;
    g_PlayerCar.drive.clutch = 10;
    g_PlayerCar.drive.drivetrainCoupled = 0;
    g_PlayerCar.drive.engineLoad = 123;
    g_PlayerCar.verticalMotionState = CAR_VERTICAL_AT_CREST;
    g_PlayerCar.verticalMotionRate = 10;
    g_PlayerCar.verticalTargetY = 1346;
    SimTickSetMultiplier(2);
    for (step = 0; step < 50; step++) {
        BeginLiveFrame();
        SimUpdatePlayerCar(&g_PlayerCar);
        if (step == 18) {
            /* Mid-last-frame of the 10-frame clutch: still decoupled on substep 0. */
            EXPECT_EQ(1, g_PlayerCar.drive.clutch);
            EXPECT_EQ(0, g_PlayerCar.drive.drivetrainCoupled);
            EXPECT_EQ(123, g_PlayerCar.drive.engineLoad);
        }
        if (step == 19) {
            /* Substep 1 of the 10th retail frame engages the clutch and clears countdown. */
            EXPECT_EQ(0, g_PlayerCar.drive.clutch);
            EXPECT_EQ(1, g_PlayerCar.drive.drivetrainCoupled);
            EXPECT_EQ(0, g_PlayerCar.drive.engineLoad);
        }
        if (step == 38) {
            /* Mid-last-frame of the 20-frame airborne reconnect: still airborne on substep 0. */
            EXPECT_EQ(1, g_PlayerCar.drive.jumpTimer);
            EXPECT_EQ(CAR_MOTION_AIRBORNE, g_PlayerCar.drive.motionState);
            EXPECT_EQ(400, g_PlayerCar.drive.shiftRpmDelta);
        }
        if (step == 39) {
            /* Substep 1 of the 20th retail frame finishes airborne motion with jumpTimer == 0. */
            EXPECT_EQ(0, g_PlayerCar.drive.jumpTimer);
            EXPECT_EQ(CAR_MOTION_DRIVING, g_PlayerCar.drive.motionState);
            EXPECT_EQ(0, g_PlayerCar.drive.shiftRpmDelta);
        }
    }
    EXPECT_EQ(CAR_VERTICAL_FALLING, g_PlayerCar.verticalMotionState);
    EXPECT_EQ(retail.verticalMotionRate, g_PlayerCar.verticalMotionRate);
    EXPECT_EQ(retail.verticalMotionTimer, g_PlayerCar.verticalMotionTimer);
    EXPECT_EQ(retail.y, g_PlayerCar.y);
    EXPECT_EQ(retail.drive.targetHeading, g_PlayerCar.drive.targetHeading & ANGLE_MASK);
}

static void CounterSteerRightStep(PlayerCarRuntime *car) {
    car->drive.trackCurveMode = 1;
    if (car->drive.steerPos < 0) {
        car->drive.steerPos = 0;
    } else if (car->drive.steerPos < 4096) {
        car->drive.steerPos += 1536;
    }
    car->steeringAngle = -car->drive.steerPos;
}

static void TestCounterSteerSnap(void) {
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    g_RacePhase = RACE_PHASE_ACTIVE;
    g_PlayerCar.drive.steerPos = -4096;
    g_PlayerCar.steeringAngle = 4096;
    s_CustomPlayerStep = CounterSteerRightStep;
    SimTickSetMultiplier(2);

    /* Substep 0: counter-steer snaps steerPos and steeringAngle to 0 immediately. */
    BeginLiveFrame();
    SimUpdatePlayerCar(&g_PlayerCar);
    EXPECT_EQ(0, g_PlayerCar.drive.steerPos);
    EXPECT_EQ(0, g_PlayerCar.steeringAngle);

    /* Substep 1: immediately begins stepping in the new direction (+768). */
    BeginLiveFrame();
    SimUpdatePlayerCar(&g_PlayerCar);
    EXPECT_EQ(768, g_PlayerCar.drive.steerPos);
    EXPECT_EQ(-768, g_PlayerCar.steeringAngle);

    s_CustomPlayerStep = NULL;
}

static void DriftTransitionStep(PlayerCarRuntime *car) {
    if (car->drive.motionState == CAR_MOTION_DRIVING) {
        car->drive.motionState = CAR_MOTION_TAKEOFF;
        car->drive.launchEnergy = 24000;
        car->drive.spinRate = 3200;
        car->drive.bodyLiftOffset = 0;
        car->drive.coastFrames = 0;
    } else if (car->drive.motionState == CAR_MOTION_TAKEOFF) {
        car->drive.motionState = CAR_MOTION_AIRBORNE;
        car->drive.jumpTimer = 20;
        car->drive.yawOffset = 512;
        car->drive.launchSpeed = 640;
        car->drive.drivetrainTorque = 960000;
        car->drive.engineLoad = 180;
        car->drive.spinRate = 0;
        car->headingAngle = 1024;
        g_ShiftSoundLevel = 1;
    } else if (car->drive.motionState == CAR_MOTION_AIRBORNE) {
        car->drive.jumpTimer--;
        car->drive.yawOffset = car->drive.yawOffset * 31 / 32;
        car->drive.launchSpeed = car->drive.launchSpeed * 31 / 32;
        car->drive.bodyLiftOffset = (s16)(car->drive.bodyLiftOffset * 2 / 3);
        if (car->drive.jumpTimer <= 0) {
            g_ShiftSoundLevel = 0;
            car->drive.shiftRpmDelta = 0;
            car->drive.yawOffset = 0;
            car->drive.launchSpeed = 0;
            car->drive.bodyLiftOffset = 0;
            car->drive.motionState = CAR_MOTION_DRIVING;
        }
    }
}

static void TestDriftTakeoffAndAirborneRecovery(void) {
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    g_RacePhase = RACE_PHASE_ACTIVE;
    g_PlayerCar.drive.motionState = CAR_MOTION_DRIVING;
    g_PlayerCar.drive.coastFrames = 14;
    g_PlayerCar.drive.bodyLiftOffset = 50;
    g_PlayerCar.headingAngle = 200;
    s_CustomPlayerStep = DriftTransitionStep;
    SimTickSetMultiplier(2);

    /* Transition DRIVING -> TAKEOFF preserves full initial launchEnergy and spinRate. */
    BeginLiveFrame();
    SimUpdatePlayerCar(&g_PlayerCar);
    EXPECT_EQ(CAR_MOTION_TAKEOFF, g_PlayerCar.drive.motionState);
    EXPECT_EQ(24000, g_PlayerCar.drive.launchEnergy);
    EXPECT_EQ(3200, g_PlayerCar.drive.spinRate);
    EXPECT_EQ(0, g_PlayerCar.drive.bodyLiftOffset);
    EXPECT_EQ(0, g_PlayerCar.drive.coastFrames);

    /* Transition TAKEOFF -> AIRBORNE preserves yawOffset, launchSpeed, torque, heading, spinRate=0. */
    BeginLiveFrame();
    SimUpdatePlayerCar(&g_PlayerCar);
    EXPECT_EQ(CAR_MOTION_AIRBORNE, g_PlayerCar.drive.motionState);
    EXPECT_EQ(20, g_PlayerCar.drive.jumpTimer);
    EXPECT_EQ(512, g_PlayerCar.drive.yawOffset);
    EXPECT_EQ(640, g_PlayerCar.drive.launchSpeed);
    EXPECT_EQ(960000, g_PlayerCar.drive.drivetrainTorque);
    EXPECT_EQ(180, g_PlayerCar.drive.engineLoad);
    EXPECT_EQ(0, g_PlayerCar.drive.spinRate);
    EXPECT_EQ(1024, g_PlayerCar.headingAngle);

    /* Fast-forward to jumpTimer == 1 entering a retail pair. */
    g_PlayerCar.drive.jumpTimer = 1;
    g_PlayerCar.drive.shiftRpmDelta = 250;
    g_PlayerCar.drive.yawOffset = 320;
    g_PlayerCar.drive.launchSpeed = 320;
    g_PlayerCar.drive.bodyLiftOffset = 30;
    g_ShiftSoundLevel = 1;

    /* Substep 0 holds jumpTimer at 1 and keeps airborne state + decayed values. */
    BeginLiveFrame();
    SimUpdatePlayerCar(&g_PlayerCar);
    EXPECT_EQ(1, g_PlayerCar.drive.jumpTimer);
    EXPECT_EQ(CAR_MOTION_AIRBORNE, g_PlayerCar.drive.motionState);
    EXPECT_EQ(250, g_PlayerCar.drive.shiftRpmDelta);
    EXPECT_EQ(1, g_ShiftSoundLevel);
    EXPECT_EQ(315, g_PlayerCar.drive.yawOffset);
    EXPECT_EQ(315, g_PlayerCar.drive.launchSpeed);
    EXPECT_EQ(25, g_PlayerCar.drive.bodyLiftOffset);

    /* Substep 1 finishes airborne motion and commits zeroed fields. */
    BeginLiveFrame();
    SimUpdatePlayerCar(&g_PlayerCar);
    EXPECT_EQ(0, g_PlayerCar.drive.jumpTimer);
    EXPECT_EQ(CAR_MOTION_DRIVING, g_PlayerCar.drive.motionState);
    EXPECT_EQ(0, g_PlayerCar.drive.shiftRpmDelta);
    EXPECT_EQ(0, g_ShiftSoundLevel);
    EXPECT_EQ(0, g_PlayerCar.drive.yawOffset);
    EXPECT_EQ(0, g_PlayerCar.drive.launchSpeed);
    EXPECT_EQ(0, g_PlayerCar.drive.bodyLiftOffset);

    s_CustomPlayerStep = NULL;
}

static void StandingStartStep(PlayerCarRuntime *car) {
    if (car->drive.motionState == CAR_MOTION_STANDING_START) {
        if (g_StandingStartSpin >= 11) {
            car->drive.standingStartBounceX = 5;
            car->drive.standingStartBounceY = 3;
            g_StandingStartSpin -= 100;
            if (g_StandingStartSpin > 0) {
                return;
            }
        }
        car->drive.standingStartBounceX = 0;
        car->drive.standingStartBounceY = 0;
        car->drive.motionState = CAR_MOTION_DRIVING;
    }
}

static void TestStandingStartWheelspin(void) {
    int step;

    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    g_PlayerCar.drive.motionState = CAR_MOTION_STANDING_START;
    g_StandingStartSpin = 250;
    s_CustomPlayerStep = StandingStartStep;
    SimTickSetMultiplier(2);

    /* In retail, 250 with grip=100 spins for 3 frames (250->150->50->-50).
     * In double-rate mode, it should spin for 6 substeps (3 pairs) and zero
     * bounce offsets on exit. */
    for (step = 0; step < 5; step++) {
        BeginLiveFrame();
        SimUpdatePlayerCar(&g_PlayerCar);
        EXPECT_EQ(CAR_MOTION_STANDING_START, g_PlayerCar.drive.motionState);
    }
    BeginLiveFrame();
    SimUpdatePlayerCar(&g_PlayerCar);
    EXPECT_EQ(CAR_MOTION_DRIVING, g_PlayerCar.drive.motionState);
    EXPECT_EQ(0, g_PlayerCar.drive.standingStartBounceX);
    EXPECT_EQ(0, g_PlayerCar.drive.standingStartBounceY);

    s_CustomPlayerStep = NULL;
}

static void CounterResetStep(PlayerCarRuntime *car) {
    (void)car;
    g_DriveBoostTimer--;
    g_AutoShiftCooldown = 0;
    g_GripLossTimer = 0;
    g_SteerHoldFrames = 0;
}

static void ChaseInitOrResetCameraStep(Camera *camera, CameraViewMode mode,
                                       GameCarRuntime *car) {
    (void)mode;
    if (camera->previousMode != CAMERA_VIEW_CHASE) {
        camera->chase.previousYaw = car->bodyYaw & ANGLE_MASK;
        camera->chase.rampNeg = 0;
        camera->chase.rampPos = 0;
        camera->previousMode = CAMERA_VIEW_CHASE;
    } else {
        camera->chase.previousYaw = (camera->chase.previousYaw + 20) & ANGLE_MASK;
        camera->chase.rampPos = 0;
        camera->chase.rampNeg = 0;
    }
}

static void TestCounterResetsAndChaseCameraInit(void) {
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    memset(&g_Camera, 0, sizeof(g_Camera));
    g_DriveBoostTimer = 1;
    g_AutoShiftCooldown = 20;
    g_GripLossTimer = 15;
    g_SteerHoldFrames = 45;
    s_CustomPlayerStep = CounterResetStep;
    s_CustomCameraStep = ChaseInitOrResetCameraStep;
    SimTickSetMultiplier(2);

    /* Substep 0 holds g_DriveBoostTimer at 1, but commits immediate resets of
     * g_AutoShiftCooldown, g_GripLossTimer, and g_SteerHoldFrames. */
    BeginLiveFrame();
    SimUpdatePlayerCar(&g_PlayerCar);
    EXPECT_EQ(1, g_DriveBoostTimer);
    EXPECT_EQ(0, g_AutoShiftCooldown);
    EXPECT_EQ(0, g_GripLossTimer);
    EXPECT_EQ(0, g_SteerHoldFrames);

    /* Entering CAMERA_VIEW_CHASE from another camera mode preserves target yaw
     * without blending against stale chase state. */
    g_Camera.previousMode = CAMERA_VIEW_CAR;
    g_Camera.chase.previousYaw = 100;
    g_Camera.chase.rampPos = 64;
    g_Camera.chase.rampNeg = 32;
    g_PlayerCar.bodyYaw = 2048;
    SimUpdateCamera(&g_Camera, CAMERA_VIEW_CHASE, AsRivalCar(&g_PlayerCar));
    EXPECT_EQ(2048, g_Camera.chase.previousYaw);
    EXPECT_EQ(0, g_Camera.chase.rampPos);
    EXPECT_EQ(0, g_Camera.chase.rampNeg);

    /* Substep 1 decrements g_DriveBoostTimer to 0 and preserves ramp resets. */
    g_Camera.chase.rampPos = 40;
    g_Camera.chase.rampNeg = 24;
    BeginLiveFrame();
    SimUpdatePlayerCar(&g_PlayerCar);
    EXPECT_EQ(0, g_DriveBoostTimer);
    SimUpdateCamera(&g_Camera, CAMERA_VIEW_CHASE, AsRivalCar(&g_PlayerCar));
    EXPECT_EQ(2058, g_Camera.chase.previousYaw);
    EXPECT_EQ(0, g_Camera.chase.rampPos);
    EXPECT_EQ(0, g_Camera.chase.rampNeg);

    s_CustomPlayerStep = NULL;
    s_CustomCameraStep = NULL;
}

static void WheelAndCollisionPlayerStep(PlayerCarRuntime *car) {
    /* Step of +3000 (> ANGLE_HALF_TURN) with CAR_WHEEL_BLUR_FLAG set. */
    car->wheelRotation =
        (((u32)car->wheelRotation + 3000u) & ANGLE_MASK) | CAR_WHEEL_BLUR_FLAG;
    g_Cars[0].speed /= 2;
    g_Cars[0].acceleration /= 2;
    g_Cars[0].collisionFlag = 1;
}

static void RivalSlideAndAvoidanceStep(void) {
    GameCarRuntime *car = &g_Cars[0];

    car->aiLateralOffset = (s16)(car->aiLateralOffset + 8);
    g_RivalCueCooldowns[0] = (s16)(g_RivalCueCooldowns[0] + 1);
    if (car->slideInput == 0 && car->yawRate == 0) {
        car->slideInput = 128;
    } else if (car->slideInput == 0) {
        car->yawRate = car->yawRate * 15 / 16;
        if (car->yawRate == 0) {
            car->slideActive = 0;
        }
    }
}

static void TestWheelRotationAndRivalSlide(void) {
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    memset(g_Cars, 0, sizeof(g_Cars));
    memset(g_RivalCueCooldowns, 0, sizeof(g_RivalCueCooldowns));
    g_Cars[0].activeFlag = 1;
    g_Cars[0].speed = 800;
    g_Cars[0].acceleration = 40;
    g_Cars[0].slideActive = 1;
    g_PlayerCar.wheelRotation = 3500;
    s_CustomPlayerStep = WheelAndCollisionPlayerStep;
    s_CustomRivalStep = RivalSlideAndAvoidanceStep;
    SimTickSetMultiplier(2);

    /* Substep 0: wheel advances forward by +1500 (3500 + 1500 = 904 mod 4096)
     * and keeps CAR_WHEEL_BLUR_FLAG; rival slideInput keeps full seed 128. */
    BeginLiveFrame();
    SimUpdatePlayerCar(&g_PlayerCar);
    EXPECT_EQ(904 | CAR_WHEEL_BLUR_FLAG, g_PlayerCar.wheelRotation);
    EXPECT_EQ(600, g_Cars[0].speed);
    EXPECT_EQ(30, g_Cars[0].acceleration);

    g_Cars[0].collisionFlag = 0;
    SimUpdateRaceCars();
    EXPECT_EQ(128, g_Cars[0].slideInput);
    EXPECT_EQ(4, g_Cars[0].aiLateralOffset);
    EXPECT_EQ(0, g_RivalCueCooldowns[0]);

    BeginLiveFrame(); /* substep 1 of first pair */
    SimUpdateRaceCars();
    EXPECT_EQ(8, g_Cars[0].aiLateralOffset);
    EXPECT_EQ(1, g_RivalCueCooldowns[0]);

    /* Test SettleSlide at yawRate == -1: substep 0 holds yawRate == -1 and
     * keeps slideActive == 1 so substep 1 finishes settling to 0. */
    g_Cars[0].slideInput = 0;
    g_Cars[0].yawRate = -1;
    g_Cars[0].slideActive = 1;
    BeginLiveFrame(); /* substep 0 of second pair */
    SimUpdateRaceCars();
    EXPECT_EQ(-1, g_Cars[0].yawRate);
    EXPECT_EQ(1, g_Cars[0].slideActive);

    BeginLiveFrame(); /* substep 1 of second pair */
    SimUpdateRaceCars();
    EXPECT_EQ(0, g_Cars[0].yawRate);
    EXPECT_EQ(0, g_Cars[0].slideActive);
    EXPECT_EQ(16, g_Cars[0].aiLateralOffset);
    EXPECT_EQ(2, g_RivalCueCooldowns[0]);

    s_CustomPlayerStep = NULL;
    s_CustomRivalStep = NULL;
}

static void KnockbackAndLandingStep(PlayerCarRuntime *car) {
    /* Re-trigger a 15-frame collision knockback while a 24-frame wall
     * knockback is already active, and land from CAR_VERTICAL_FALLING onto a
     * flat road surface (bodyPitch = 12, bodyRoll = -8). */
    car->motionActive = 1;
    car->motionTimer = 15;
    car->velocityX = -32;
    car->velocityZ = 48;
    car->verticalMotionState = CAR_VERTICAL_GROUNDED;
    car->y = 500;
    car->bodyPitch = 12;
    car->bodyRoll = -8;
    car->verticalPitch = 0;
    car->verticalRoll = 0;
    g_EngineRpm += 400;
    g_EngineRpmSnapshot = g_EngineRpm;
}

static void ShiftRpmAndLaunchTargetStep(PlayerCarRuntime *car) {
    if (car->drive.motionState == CAR_MOTION_TAKEOFF) {
        car->bodyYaw = 1020;
        car->drive.targetHeading = car->bodyYaw;
    } else if (car->drive.clutch > 1) {
        s16 countdown = (s16)(car->drive.clutch - 1);
        car->drive.clutch = countdown;
        car->drive.engineRpm = 5000 + countdown * 500;
    }
}

static void TestKnockbackRetriggerLandingAndShiftRpm(void) {
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    g_PlayerCar.motionActive = 1;
    g_PlayerCar.motionTimer = 24;
    g_PlayerCar.velocityX = 20;
    g_PlayerCar.velocityZ = -10;
    g_PlayerCar.verticalMotionState = CAR_VERTICAL_FALLING;
    g_PlayerCar.y = 480;
    g_PlayerCar.verticalPitch = 260;
    g_PlayerCar.verticalRoll = -140;
    g_PlayerCar.bodyPitch = 260;
    g_PlayerCar.bodyRoll = -140;
    g_EngineRpm = 4000;
    g_EngineRpmSnapshot = 4000;
    s_CustomPlayerStep = KnockbackAndLandingStep;
    SimTickSetMultiplier(2);

    BeginLiveFrame();
    SimUpdatePlayerCar(&g_PlayerCar);
    /* Newly triggered 15-frame knockback keeps its full timer and velocity. */
    EXPECT_EQ(1, g_PlayerCar.motionActive);
    EXPECT_EQ(15, g_PlayerCar.motionTimer);
    EXPECT_EQ(-32, g_PlayerCar.velocityX);
    EXPECT_EQ(48, g_PlayerCar.velocityZ);
    /* Landing commits grounded y, bodyPitch, and bodyRoll without blending in
     * the airborne verticalPitch/verticalRoll. */
    EXPECT_EQ(CAR_VERTICAL_GROUNDED, g_PlayerCar.verticalMotionState);
    EXPECT_EQ(500, g_PlayerCar.y);
    EXPECT_EQ(12, g_PlayerCar.bodyPitch);
    EXPECT_EQ(-8, g_PlayerCar.bodyRoll);
    EXPECT_EQ(0, g_PlayerCar.verticalPitch);
    EXPECT_EQ(0, g_PlayerCar.verticalRoll);
    /* g_EngineRpmSnapshot stays synchronized with blended g_EngineRpm. */
    EXPECT_EQ(4200, g_EngineRpm);
    EXPECT_EQ(4200, g_EngineRpmSnapshot);

    /* During CAR_MOTION_TAKEOFF, targetHeading stays locked to blended bodyYaw. */
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    g_PlayerCar.drive.motionState = CAR_MOTION_TAKEOFF;
    g_PlayerCar.bodyYaw = 1000;
    g_PlayerCar.drive.targetHeading = 1080;
    s_CustomPlayerStep = ShiftRpmAndLaunchTargetStep;
    BeginLiveFrame();
    SimUpdatePlayerCar(&g_PlayerCar);
    EXPECT_EQ(1010, g_PlayerCar.bodyYaw);
    EXPECT_EQ(1010, g_PlayerCar.drive.targetHeading);

    /* During gear shift interpolation, substep 0 blends halfway and substep 1
     * lands on the exact retail interpolated RPM for the decremented clutch. */
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    g_PlayerCar.drive.motionState = CAR_MOTION_DRIVING;
    g_PlayerCar.drive.gear = 2;
    g_PlayerCar.drive.gearDisp = 2;
    g_PlayerCar.drive.clutch = 10;
    g_PlayerCar.drive.engineRpm = 10000;
    BeginLiveFrame(); /* substep 0: clutch holds at 10, RPM blends to 9750 */
    SimUpdatePlayerCar(&g_PlayerCar);
    EXPECT_EQ(10, g_PlayerCar.drive.clutch);
    EXPECT_EQ(9750, g_PlayerCar.drive.engineRpm);

    BeginLiveFrame(); /* substep 1: clutch steps to 9, RPM reaches 9500 */
    SimUpdatePlayerCar(&g_PlayerCar);
    EXPECT_EQ(9, g_PlayerCar.drive.clutch);
    EXPECT_EQ(9500, g_PlayerCar.drive.engineRpm);

    s_CustomPlayerStep = NULL;
}

int main(void) {
    TestHalfDelta();
    TestBlendFields();
    TestFrameSequence();
    TestPadEdges();
    TestRetailPassThrough();
    TestDoubleTracksRetail();
    TestAirborneDrivetrainAndCrestFall();
    TestRivalsAndCamera();
    TestCounterSteerSnap();
    TestDriftTakeoffAndAirborneRecovery();
    TestStandingStartWheelspin();
    TestCounterResetsAndChaseCameraInit();
    TestWheelRotationAndRivalSlide();
    TestKnockbackRetriggerLandingAndShiftRpm();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
