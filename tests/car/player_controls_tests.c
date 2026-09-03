#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/state.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

u8 g_PadType;
s16 g_SteerHoldFrames;

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
    g_PadType = PAD_TYPE_DIGITAL;
    g_SteerHoldFrames = 0;
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
    UpdatePlayerControlFeedback(&car);
    CHECK(car.wheelRotation == 700);

    Reset(&car);
    car.speed = 801;
    car.wheelRotation = 4000;
    UpdatePlayerControlFeedback(&car);
    CHECK(car.wheelRotation == (((4000 + 2403) & 0xFFF) | 0x1000));

    Reset(&car);
    car.speed = 2000;
    UpdatePlayerControlFeedback(&car);
    CHECK(car.wheelRotation == (0x249 | 0x1000));

    Reset(&car);
    car.steeringAngle = 5000;
    UpdatePlayerControlFeedback(&car);
    CHECK(car.steeringAngle == 4096 && g_SteerHoldFrames == 1);

    Reset(&car);
    car.steeringAngle = -5000;
    UpdatePlayerControlFeedback(&car);
    CHECK(car.steeringAngle == -4096 && g_SteerHoldFrames == 1);

    Reset(&car);
    car.steeringAngle = -4096;
    UpdatePlayerControlFeedback(&car);
    CHECK(car.steeringAngle == -4096 && g_SteerHoldFrames == 1);

    Reset(&car);
    g_PadType = PAD_TYPE_NEGCON;
    car.steeringAngle = 5000;
    car.drive.steerPos = -4096;
    UpdatePlayerControlFeedback(&car);
    CHECK(car.steeringAngle == 4096 && g_SteerHoldFrames == 0);
    car.drive.steerPos = -4097;
    UpdatePlayerControlFeedback(&car);
    CHECK(g_SteerHoldFrames == 1);

    Reset(&car);
    g_PadType = PAD_TYPE_NEGCON;
    car.steeringAngle = 0;
    g_SteerHoldFrames = 20;
    UpdatePlayerControlFeedback(&car);
    CHECK(g_SteerHoldFrames == -10);

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
    g_SteerHoldFrames = INT16_MAX;
    UpdatePlayerControlFeedback(&car);
    CHECK(g_SteerHoldFrames == INT16_MIN);

    if (s_failures != 0) {
        printf("%d player control checks failed\n", s_failures);
        return 1;
    }
    puts("player steering and wheel feedback preserve their thresholds");
    return 0;
}
