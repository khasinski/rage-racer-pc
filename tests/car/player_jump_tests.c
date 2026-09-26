#include "game/car.h"
#include "game/car_internal.h"
#include "game/car_motion_internal.h"
#include "game/race.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

GameCarSpec *g_CarSpec;
s16 g_RacePhase;

static GameCarSpec s_spec;
static int s_soundCalls;
static int s_soundCue;
static int s_failures;


void PlaySoundCue(s32 cue) {
    s_soundCalls++;
    s_soundCue = cue;
}

static void StepJump(PlayerCarRuntime *car, s32 ground) {
    const s32 frames = StepPlayerJump(car, g_CarSpec, ground)
        ? car->verticalMotionTimer : 0;
    PlayPlayerLandingCue(frames, g_RacePhase <= RACE_PHASE_ACTIVE);
}

static void Reset(PlayerCarRuntime *car) {
    memset(car, 0, sizeof(*car));
    memset(&s_spec, 0, sizeof(s_spec));
    s_spec.gearRatio[2] = 1000;
    s_spec.gearLoad[2] = 200;
    g_CarSpec = &s_spec;
    g_RacePhase = 2;
    car->drive.shiftTargetRpm = 0;
    s_soundCalls = 0;
    s_soundCue = 0;
}

#define CHECK(condition) do {                                                \
    if (!(condition)) {                                                      \
        printf("FAIL line %d: %s\n", __LINE__, #condition);                 \
        s_failures++;                                                        \
    }                                                                        \
} while (0)

int main(void) {
    PlayerCarRuntime car;

    Reset(&car);
    car.y = 123;
    StepJump(&car, 500);
    CHECK(car.y == 123 && car.verticalMotionTimer == 0 && car.motionModeTimer == 0);

    Reset(&car);
    car.verticalMotionState = CAR_VERTICAL_RISING;
    car.verticalMotionRate = -20;
    car.y = 100;
    StepJump(&car, 500);
    CHECK(car.verticalMotionState == CAR_VERTICAL_RISING);
    CHECK(car.verticalMotionTimer == 1 && car.y == 80);

    Reset(&car);
    car.verticalMotionState = CAR_VERTICAL_AT_CREST;
    car.verticalMotionRate = 10;
    car.verticalTargetY = 100;
    StepJump(&car, 105);
    CHECK(car.verticalMotionState == CAR_VERTICAL_AT_CREST && car.y == 100);

    Reset(&car);
    car.verticalMotionState = CAR_VERTICAL_AT_CREST;
    car.verticalMotionRate = 10;
    car.verticalTargetY = 100;
    StepJump(&car, 200);
    CHECK(car.verticalMotionState == CAR_VERTICAL_FALLING &&
          car.verticalMotionRate == 1);
    CHECK(car.y == 100);

    Reset(&car);
    car.verticalMotionState = CAR_VERTICAL_FALLING;
    car.verticalMotionRate = 0;
    car.verticalTargetY = 100;
    StepJump(&car, 500);
    CHECK(car.verticalMotionState == CAR_VERTICAL_FALLING && car.y == 102);

    Reset(&car);
    car.verticalMotionState = CAR_VERTICAL_FALLING;
    car.verticalMotionTimer = 18;
    car.verticalMotionRate = 0;
    car.verticalTargetY = 0;
    car.verticalPitch = 12;
    car.verticalRoll = -9;
    car.speed = 1168;
    car.headingAngle = 0x345;
    car.drive.gear = 2;
    car.drive.manual = 1;
    car.drive.motionState = CAR_MOTION_DRIVING;
    car.drive.engineRpm = 1200;
    StepJump(&car, 100);
    CHECK(car.verticalMotionState == CAR_VERTICAL_GROUNDED && car.y == 108);
    CHECK(car.verticalPitch == 0 && car.verticalRoll == 0);
    CHECK(car.motionMode == CAR_BODY_KICK_LANDING && car.motionModeTimer == 30);
    CHECK(s_soundCalls == 1 && s_soundCue == 0xE);
    CHECK(car.drive.motionState == CAR_MOTION_AIRBORNE);
    CHECK(car.drive.jumpTimer == 20 && car.drive.shiftTargetRpm == 1600);
    CHECK(car.drive.engineLoad == 2);
    CHECK(car.drive.launchHeading == car.headingAngle);

    Reset(&car);
    s_spec.gearRatio[2] = 0;
    car.verticalMotionState = CAR_VERTICAL_FALLING;
    car.verticalMotionTimer = 2;
    car.verticalTargetY = 0;
    car.speed = 1168;
    car.drive.gear = 2;
    car.drive.manual = 1;
    car.drive.motionState = CAR_MOTION_DRIVING;
    StepJump(&car, 10);
    CHECK(car.drive.motionState == CAR_MOTION_AIRBORNE);
    CHECK(car.drive.shiftTargetRpm == 1600000);

    Reset(&car);
    car.verticalMotionState = CAR_VERTICAL_RISING;
    car.verticalMotionTimer = INT16_MAX;
    car.verticalMotionRate = INT16_MAX;
    StepJump(&car, INT_MAX);
    CHECK(car.verticalMotionTimer == INT16_MIN);

    Reset(&car);
    s_spec.gearRatio[2] = 1;
    s_spec.gearLoad[2] = INT_MAX;
    car.speed = 100000;
    car.drive.gear = 2;
    car.drive.engineRpm = INT_MIN;
    car.drive.manual = 0;
    PrepareAirborneDrivetrain(&car, &s_spec);
    CHECK(car.drive.motionState == CAR_MOTION_AIRBORNE);
    CHECK(car.drive.jumpTimer == 20);
    CHECK(car.drive.drivetrainTorque == 15107194);
    CHECK(car.drive.shiftTargetRpm == 136980000);
    CHECK(car.drive.shiftRpmDelta == 9760);
    CHECK(car.drive.engineLoad == -1029);

    Reset(&car);
    car.verticalMotionState = CAR_VERTICAL_RISING;
    car.verticalMotionTimer = -1;
    car.verticalMotionRate = 0;
    car.y = INT_MAX;
    car.drive.motionState = CAR_MOTION_AIRBORNE;
    StepJump(&car, INT_MAX);
    CHECK(car.verticalMotionState == CAR_VERTICAL_GROUNDED);
    CHECK(car.y == INT_MIN + CAR_WHEEL_GROUND_OFFSET - 1);
    CHECK(car.motionMode == CAR_BODY_KICK_LANDING && car.motionModeTimer == 30);

    if (s_failures != 0) {
        printf("%d player jump checks failed\n", s_failures);
        return 1;
    }
    puts("player jump phases and landing drivetrain are bounded");
    return 0;
}
