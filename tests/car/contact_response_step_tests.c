#include "game/car_motion_internal.h"
#include "game/track.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

int main(void) {
    const GameTrackPoint straight = {.angle = 0};
    const GameTrackPoint turn = {.angle = -0x400};
    PlayerCarRuntime initial = {0};
    initial.headingAngle = 0xC00;
    initial.speed = 200;
    initial.drive.launchEnergy = 10000;
    initial.drive.drivetrainTorque = 10000;
    initial.drive.engineLoad = 1000;
    initial.drive.shiftTargetRpm = 2000;
    PlayerCarRuntime first = initial;
    CHECK(ApplyCarContactResponse(&first, &straight, 1, 0) == 0);
    CHECK(first.speed == 174 && first.drive.drivetrainTorque == 8500);
    CHECK(first.drive.launchEnergy == 5000 && first.drive.engineLoad == 850);
    CHECK(first.drive.shiftTargetRpm == 1700);
    PlayerCarRuntime second = initial;
    CHECK(ApplyCarContactResponse(&second, &turn, 1, 0) == 1024);
    CHECK(second.speed == 94 && second.drive.drivetrainTorque == 6500);
    CHECK(second.drive.engineLoad == 650 && second.drive.shiftTargetRpm == 1300);
    CHECK(first.speed == 174);
    PlayerCarRuntime restored = initial;
    ApplyCarContactResponse(&restored, &straight, 1, 0);
    CHECK(memcmp(&first, &restored, sizeof(first)) == 0);

    restored = initial;
    CHECK(ApplyCarContactResponse(&restored, NULL, 1, 0) == -1);
    CHECK(memcmp(&restored, &initial, sizeof(initial)) == 0);
    CHECK(ApplyCarContactResponse(&restored, NULL, 1, 1) == -1);
    CHECK(restored.speed == 194 && restored.drive.drivetrainTorque == 9800);
    CHECK(restored.drive.launchEnergy == 9000 && restored.drive.engineLoad == 950);
    restored = initial;
    restored.speed = 80;
    ApplyCarContactResponse(&restored, NULL, 0, 1);
    CHECK(restored.speed == 80 && restored.drive.drivetrainTorque == 10000);

    restored = initial;
    restored.y = 100;
    restored.drive.standingStartBounceY = 7;
    restored.verticalMotionTimer = 7;
    BeginCarBodyKick(AsRivalCar(&restored), CAR_BODY_KICK_LANDING, 0, 0);
    CHECK(ApplyCarContactResponse(&restored, NULL, 0, 0) == -1);
    CHECK(restored.y == 107 && restored.motionModeTimer == 29);
    CHECK(restored.bodyPitch != 0);
    puts("contact response step tests passed");
    return 0;
}
