#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"

#include <stdio.h>
#include <string.h>

GameCarSpec *g_CarSpec;
s16 g_RacePhase;
s32 g_ShiftSoundLevel;
s32 g_ShiftTargetRpm;

static GameCarSpec s_spec;
static int s_kickCalls;
static int s_kickMode;
static int s_soundCalls;
static int s_soundCue;
static int s_failures;

void StartCarBodyKick(GameCarRuntime *car, s32 mode) {
    (void)car;
    s_kickCalls++;
    s_kickMode = mode;
}

void PlaySoundCue(s32 cue) {
    s_soundCalls++;
    s_soundCue = cue;
}

static void Reset(PlayerCarRuntime *car) {
    memset(car, 0, sizeof(*car));
    memset(&s_spec, 0, sizeof(s_spec));
    s_spec.gearRatio[2] = 1000;
    s_spec.gearLoad[2] = 200;
    g_CarSpec = &s_spec;
    g_RacePhase = 2;
    g_ShiftSoundLevel = 0;
    g_ShiftTargetRpm = 0;
    s_kickCalls = 0;
    s_kickMode = 0;
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
    UpdatePlayerJump(&car, 500);
    CHECK(car.y == 123 && car.verticalMotionTimer == 0 && s_kickCalls == 0);

    Reset(&car);
    car.verticalMotionState = 1;
    car.verticalMotionRate = -20;
    car.y = 100;
    UpdatePlayerJump(&car, 500);
    CHECK(car.verticalMotionState == 1);
    CHECK(car.verticalMotionTimer == 1 && car.y == 80);

    Reset(&car);
    car.verticalMotionState = 2;
    car.verticalMotionRate = 10;
    car.verticalTargetY = 100;
    UpdatePlayerJump(&car, 105);
    CHECK(car.verticalMotionState == 2 && car.y == 100);

    Reset(&car);
    car.verticalMotionState = 2;
    car.verticalMotionRate = 10;
    car.verticalTargetY = 100;
    UpdatePlayerJump(&car, 200);
    CHECK(car.verticalMotionState == 3 && car.verticalMotionRate == 1);
    CHECK(car.y == 100);

    Reset(&car);
    car.verticalMotionState = 3;
    car.verticalMotionRate = 0;
    car.verticalTargetY = 100;
    UpdatePlayerJump(&car, 500);
    CHECK(car.verticalMotionState == 3 && car.y == 102);

    Reset(&car);
    car.verticalMotionState = 3;
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
    UpdatePlayerJump(&car, 100);
    CHECK(car.verticalMotionState == 0 && car.y == 108);
    CHECK(car.verticalPitch == 0 && car.verticalRoll == 0);
    CHECK(s_kickCalls == 1 && s_kickMode == 1);
    CHECK(s_soundCalls == 1 && s_soundCue == 0xE);
    CHECK(car.drive.motionState == CAR_MOTION_AIRBORNE);
    CHECK(car.drive.jumpTimer == 20 && g_ShiftTargetRpm == 1600);
    CHECK(car.drive.engineLoad == 2);
    CHECK(car.drive.launchHeading == car.headingAngle);

    Reset(&car);
    s_spec.gearRatio[2] = 0;
    car.verticalMotionState = 3;
    car.verticalMotionTimer = 2;
    car.verticalTargetY = 0;
    car.speed = 1168;
    car.drive.gear = 2;
    car.drive.manual = 1;
    car.drive.motionState = CAR_MOTION_DRIVING;
    UpdatePlayerJump(&car, 10);
    CHECK(car.drive.motionState == CAR_MOTION_AIRBORNE);
    CHECK(g_ShiftTargetRpm == 1600000);

    if (s_failures != 0) {
        printf("%d player jump checks failed\n", s_failures);
        return 1;
    }
    puts("player jump phases and landing drivetrain are bounded");
    return 0;
}
