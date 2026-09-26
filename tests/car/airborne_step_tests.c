#include "game/car_drive.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void) {
    PlayerCarRuntime first = {0};
    first.speed = 1000;
    first.drive.motionState = CAR_MOTION_AIRBORNE;
    first.drive.yawOffset = 2048;
    first.drive.spinRate = 160;
    first.drive.launchSpeed = 320;
    first.drive.bodyLiftOffset = 9;
    first.drive.jumpTimer = 10;
    first.drive.shiftSoundLevel = 25;
    first.drive.shiftRpmDelta = 50;
    PlayerCarRuntime second = first;
    second.drive.yawOffset = -4096;
    second.drive.shiftSoundLevel = 100;
    PlayerCarRuntime alone = first;

    CHECK(StepCarAirborne(&first) == 0);
    CHECK(first.speed == 800);
    CHECK(first.drive.yawOffset == 1984);
    CHECK(first.drive.spinRate == 155);
    CHECK(first.drive.launchSpeed == 310);
    CHECK(first.drive.bodyLiftOffset == 6);
    CHECK(first.drive.coastFrames == 1);
    CHECK(first.drive.shiftSoundLevel == 25);
    StepCarAirborne(&alone);
    for (int i = 0; i < 10; i++) {
        StepCarAirborne(&second);
        StepCarAirborne(&first);
        StepCarAirborne(&alone);
    }
    CHECK(memcmp(&first, &alone, sizeof(first)) == 0);
    CHECK(second.drive.shiftSoundLevel == 100);
    PlayerCarRuntime restored = first;
    StepCarAirborne(&second);
    StepCarAirborne(&first);
    StepCarAirborne(&restored);
    CHECK(memcmp(&first, &restored, sizeof(first)) == 0);
    first.drive.jumpTimer = 0;
    CHECK(StepCarAirborne(&first) == 1);
    CHECK(first.drive.motionState == CAR_MOTION_DRIVING);
    CHECK(first.drive.shiftSoundLevel == 0 && first.drive.shiftRpmDelta == 0);
    CHECK(first.drive.yawOffset == 0 && first.drive.launchSpeed == 0);
    CHECK(first.drive.bodyLiftOffset == 0);
    CHECK(second.drive.shiftSoundLevel == 100);
    return 0;
}
