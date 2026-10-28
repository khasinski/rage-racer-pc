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
s32 g_StandingStartSpin;
s16 g_GripLossTimer;
s16 g_SteerHoldFrames;
s16 g_RacePhase;
s32 g_FrameCounter;

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
    if (car->drive.jumpTimer == 400) car->progressA += 300;
    car->speed = car->speed * 94 / 100 + 50;
    car->x += car->speed;
    car->drive.jumpTimer--;
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
        car->y = car->verticalTargetY + tick * tick;
        car->verticalPitch = (s16)(car->verticalPitch + 6);
        car->bodyPitch = car->verticalPitch;
    }
    g_GripLossTimer--;
}

void UpdatePlayerCar(PlayerCarRuntime *car) { RetailStep(car); }

void UpdateRaceCars(void) {
    int index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        if (g_Cars[index].activeFlag == -1) continue;
        g_Cars[index].speed = g_Cars[index].speed * 94 / 100 + 50;
        g_Cars[index].x += g_Cars[index].speed;
    }
}

void UpdateCamera(Camera *camera, CameraViewMode mode, GameCarRuntime *car) {
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

int main(void) {
    TestHalfDelta();
    TestBlendFields();
    TestFrameSequence();
    TestPadEdges();
    TestRetailPassThrough();
    TestDoubleTracksRetail();
    TestRivalsAndCamera();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
