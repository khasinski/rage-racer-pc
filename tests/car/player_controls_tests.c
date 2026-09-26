#include "game/car_control.h"
#include "game/integer.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

static int s_analog;

static int s_failures;

#define CHECK(condition) do {                                                \
    if (!(condition)) {                                                      \
        printf("FAIL line %d: %s\n", __LINE__, #condition);                 \
        s_failures++;                                                        \
    }                                                                        \
} while (0)

static void Reset(PlayerCarRuntime *car) {
    memset(car, 0, sizeof(*car));
    car->drive.steerPos = 4096;
    car->drive.steeringGrip = 256;
    car->drive.motionState = CAR_MOTION_DRIVING;
    car->drive.targetHeading = 100;
    s_analog = 0;
    car->drive.steerHoldFrames = 0;
}

int main(void) {
    PlayerCarRuntime car;

    Reset(&car);
    car.speed = 0;
    UpdatePlayerSteeringTarget(&car);
    CHECK(car.drive.targetHeading == 100);

    Reset(&car);
    car.speed = 128;
    UpdatePlayerSteeringTarget(&car);
    CHECK(car.drive.targetHeading == 109);

    Reset(&car);
    car.speed = 256;
    UpdatePlayerSteeringTarget(&car);
    CHECK(car.drive.targetHeading == 119);

    Reset(&car);
    car.speed = 256;
    car.drive.motionState = CAR_MOTION_STANDING_START;
    UpdatePlayerSteeringTarget(&car);
    CHECK(car.drive.targetHeading == 109);

    Reset(&car);
    car.speed = 100;
    car.wheelRotation = 400;
    UpdateCarControlFeedback(&car, s_analog);
    CHECK(car.wheelRotation == 700);

    Reset(&car);
    car.speed = 801;
    car.wheelRotation = 4000;
    UpdateCarControlFeedback(&car, s_analog);
    CHECK(car.wheelRotation == (((4000 + 2403) & 0xFFF) | 0x1000));

    Reset(&car);
    car.speed = 2000;
    UpdateCarControlFeedback(&car, s_analog);
    CHECK(car.wheelRotation == (0x249 | 0x1000));

    Reset(&car);
    car.steeringAngle = 5000;
    UpdateCarControlFeedback(&car, s_analog);
    CHECK(car.steeringAngle == 4096 && car.drive.steerHoldFrames == 1);

    Reset(&car);
    car.steeringAngle = -5000;
    UpdateCarControlFeedback(&car, s_analog);
    CHECK(car.steeringAngle == -4096 && car.drive.steerHoldFrames == 1);

    Reset(&car);
    car.steeringAngle = -4096;
    UpdateCarControlFeedback(&car, s_analog);
    CHECK(car.steeringAngle == -4096 && car.drive.steerHoldFrames == 1);

    Reset(&car);
    s_analog = 1;
    car.steeringAngle = 5000;
    car.drive.steerPos = -4096;
    UpdateCarControlFeedback(&car, s_analog);
    CHECK(car.steeringAngle == 4096 && car.drive.steerHoldFrames == 0);
    car.drive.steerPos = -4097;
    UpdateCarControlFeedback(&car, s_analog);
    CHECK(car.drive.steerHoldFrames == 1);

    Reset(&car);
    s_analog = 1;
    car.steeringAngle = 0;
    car.drive.steerHoldFrames = 20;
    UpdateCarControlFeedback(&car, s_analog);
    CHECK(car.drive.steerHoldFrames == -10);

    Reset(&car);
    car.speed = 256;
    car.drive.steerPos = INT_MAX;
    car.drive.steeringGrip = INT16_MAX;
    car.drive.targetHeading = INT_MAX;
    UpdatePlayerSteeringTarget(&car);
    CHECK(car.drive.targetHeading ==
          WrapSigned32(
              (int64_t)INT_MAX +
              WrapSigned32(
                  (int64_t)(WrapSigned32((int64_t)INT_MAX * 6) / 5) *
                  INT16_MAX) /
                  0x10000));

    Reset(&car);
    car.steeringAngle = 4096;
    car.drive.steerHoldFrames = INT16_MAX;
    UpdateCarControlFeedback(&car, s_analog);
    CHECK(car.drive.steerHoldFrames == INT16_MIN);

    PlayerCarRuntime digital;
    PlayerCarRuntime analog;
    Reset(&digital);
    Reset(&analog);
    digital.steeringAngle = 5000;
    analog.steeringAngle = 5000;
    analog.drive.steerPos = -4096;
    UpdateCarControlFeedback(&digital, 0);
    UpdateCarControlFeedback(&analog, 1);
    CHECK(digital.drive.steerHoldFrames == 1);
    CHECK(analog.drive.steerHoldFrames == 0);
    PlayerCarRuntime saved = digital;
    UpdateCarControlFeedback(&analog, 1);
    UpdateCarControlFeedback(&digital, 0);
    UpdateCarControlFeedback(&saved, 0);
    CHECK(memcmp(&digital, &saved, sizeof(digital)) == 0);
    CHECK(digital.drive.steerHoldFrames == 2);
    CHECK(analog.drive.steerHoldFrames == 0);

    if (s_failures != 0) {
        printf("%d player control checks failed\n", s_failures);
        return 1;
    }
    puts("player steering and wheel feedback preserve their thresholds");
    return 0;
}
