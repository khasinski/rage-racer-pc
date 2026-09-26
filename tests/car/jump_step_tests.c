#include "game/car_motion_internal.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

int main(void) {
    GameCarSpec firstSpec = {0};
    GameCarSpec secondSpec = {0};
    firstSpec.gearRatio[2] = 1000;
    firstSpec.gearLoad[2] = 200;
    secondSpec.gearRatio[2] = 2000;
    secondSpec.gearLoad[2] = 400;
    PlayerCarRuntime initial = {0};
    initial.verticalMotionState = CAR_VERTICAL_FALLING;
    initial.verticalMotionTimer = 18;
    initial.speed = 1168;
    initial.headingAngle = 0x345;
    initial.drive.gear = 2;
    initial.drive.manual = 1;
    initial.drive.engineRpm = 1200;
    initial.drive.motionState = CAR_MOTION_DRIVING;

    PlayerCarRuntime first = initial;
    CHECK(StepPlayerJump(&first, &firstSpec, 100) == 1);
    CHECK(first.y == 108 && first.verticalMotionState == CAR_VERTICAL_GROUNDED);
    CHECK(first.motionMode == CAR_BODY_KICK_LANDING);
    CHECK(first.motionValue == 152 && first.motionModeTimer == 30);
    CHECK(first.drive.shiftTargetRpm == 1600 && first.drive.engineLoad == 2);
    CHECK(first.drive.motionState == CAR_MOTION_AIRBORNE);
    CHECK(first.drive.launchHeading == 0x345);

    PlayerCarRuntime second = initial;
    CHECK(StepPlayerJump(&second, &secondSpec, 200) == 1);
    CHECK(second.y == 208 && second.drive.shiftTargetRpm == 800);
    CHECK(first.y == 108 && first.drive.shiftTargetRpm == 1600);
    PlayerCarRuntime restored = initial;
    StepPlayerJump(&restored, &firstSpec, 100);
    CHECK(memcmp(&first, &restored, sizeof(first)) == 0);
    CHECK(StepPlayerJump(&first, &firstSpec, 100) == 0);
    CHECK(memcmp(&first, &restored, sizeof(first)) == 0);

    initial.verticalMotionState = CAR_VERTICAL_RISING;
    initial.verticalMotionTimer = 0;
    initial.verticalMotionRate = -20;
    initial.y = 100;
    CHECK(StepPlayerJump(&initial, &firstSpec, 500) == 0);
    CHECK(initial.y == 80 && initial.verticalMotionTimer == 1);
    puts("jump step tests passed");
    return 0;
}
