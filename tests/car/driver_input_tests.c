#include "game/car_control.h"
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

int main(void) {
    GameCarSpec spec = {.topGear = 6};
    PlayerCarRuntime first = {0};
    first.drive.manual = 1;
    first.drive.gear = 2;
    first.speed = 256;
    first.drive.steeringGrip = 256;
    const DriverInput left = {
        .steering = {.mode = STEERING_DIGITAL, .left = 1},
        .throttle = 256, .brake = 32, .shiftUp = 1,
    };
    PlayerCarRuntime second = first;
    const DriverInput right = {
        .steering = {.mode = STEERING_DIGITAL, .right = 1},
        .throttle = 128, .brake = 256, .shiftDown = 1,
    };
    PlayerCarRuntime alone = first;
    ApplyDriverInput(&first, &spec, &left);
    CHECK(first.drive.gear == 3);
    CHECK(first.drive.steerPos == -1536);
    CHECK(first.drive.targetHeading == -7);
    CHECK(first.drive.acceleratorInput.value == 256 && first.drive.brakeInput == 32);
    ApplyDriverInput(&second, &spec, &right);
    CHECK(second.drive.gear == 1 && second.drive.steerPos == 1536);
    CHECK(second.drive.acceleratorInput.value == 128 && second.drive.brakeInput == 256);
    ApplyDriverInput(&alone, &spec, &left);
    CHECK(memcmp(&first, &alone, sizeof(first)) == 0);
    PlayerCarRuntime saved = first;
    const DriverInput release = {.steering.mode = STEERING_DIGITAL};
    ApplyDriverInput(&first, &spec, &release);
    ApplyDriverInput(&second, &spec, &release);
    ApplyDriverInput(&saved, &spec, &release);
    CHECK(memcmp(&first, &saved, sizeof(first)) == 0);
    CHECK(first.drive.steerPos == -512 && second.drive.steerPos == 512);
    CHECK(first.drive.acceleratorInput.value == 0 && first.drive.brakeInput == 0);
    first.verticalMotionState = CAR_VERTICAL_RISING;
    const s32 heading = first.drive.targetHeading;
    ApplyDriverInput(&first, &spec, &left);
    CHECK(first.drive.targetHeading == heading);
    /* Automatic cooldown sees the old brake, not the newly sampled one. */
    first.drive.manual = 0;
    first.drive.autoShiftCooldown = 10;
    first.drive.brakeInput = 256;
    ApplyDriverInput(&first, &spec, &release);
    CHECK(first.drive.autoShiftCooldown == 8 && first.drive.brakeInput == 0);
    return 0;
}
