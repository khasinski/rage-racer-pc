#include "game/car_drive.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#define CHECK_EQ(actual, expected) do {                                        \
    if ((actual) != (expected)) {                                               \
        fprintf(stderr, "line %d: %s = %d, expected %d\n", __LINE__, #actual, \
                (int)(actual), (int)(expected));                                \
        return 1;                                                               \
    }                                                                           \
} while (0)

int main(void) {
    GameCarSpec spec;
    CarPerformance performance = {0};
    s32 engineRpm;
    PlayerCarRuntime car;

    memset(&spec, 0, sizeof(spec));
    memset(&car, 0, sizeof(car));
    spec.revLimit = 8000;
    engineRpm = 4000;
    performance.peakRpm = 3000;
    performance.peakOutput = 1000;
    car.drive.gear = 2;
    car.drive.drivetrainTorque = 600;
    BeginCarStandingStart(&car, &spec, &performance, engineRpm);
    CHECK_EQ(car.drive.drivetrainTorque, 300);
    CHECK_EQ(car.drive.standingStartSpin, 1250);
    CHECK_EQ(car.drive.gripLossTimer, 200);

    car.drive.gear = 0;
    car.drive.drivetrainTorque = 600;
    spec.revLimit = 0;
    car.drive.gripLossTimer = 123;
    BeginCarStandingStart(&car, &spec, &performance, engineRpm);
    CHECK_EQ(car.drive.gear, 1);
    CHECK_EQ(car.drive.drivetrainTorque, 600);
    CHECK_EQ(car.drive.gripLossTimer, 0);

    spec.revLimit = 8000;
    engineRpm = 1500;
    performance.peakRpm = 3000;
    BeginCarStandingStart(&car, &spec, &performance, engineRpm);
    CHECK_EQ(car.drive.standingStartSpin, 0);

    engineRpm = 2500;
    BeginCarStandingStart(&car, &spec, &performance, engineRpm);
    CHECK_EQ(car.drive.standingStartSpin, 1500);

    car.drive.gear = CAR_FORWARD_GEAR_COUNT + 5;
    car.drive.drivetrainTorque = 600;
    engineRpm = 4000;
    BeginCarStandingStart(&car, &spec, &performance, engineRpm);
    CHECK_EQ(car.drive.gear, CAR_FORWARD_GEAR_COUNT);
    CHECK_EQ(car.drive.drivetrainTorque, 100);
    CHECK_EQ(car.drive.gripLossTimer, 200);

    memset(&car, 0, sizeof(car));
    car.drive.motionState = CAR_MOTION_STANDING_START;
    car.drive.engineRpm = 2000;
    car.drive.acceleratorInput.value = 0;
    car.drive.standingStartSpin = 1000;
    StepCarStandingStart(&car, 0, 0);
    CHECK_EQ(car.drive.standingStartSpin, 968);
    CHECK_EQ(car.drive.motionState, CAR_MOTION_STANDING_START);

    memset(&car, 0, sizeof(car));
    car.drive.gear = 1;
    spec.revLimit = 1;
    engineRpm = INT_MAX;
    performance.peakRpm = INT16_MIN;
    performance.peakOutput = 1000;
    BeginCarStandingStart(&car, &spec, &performance, engineRpm);
    CHECK_EQ(car.drive.standingStartSpin, 655340000);

    PlayerCarRuntime first = {0};
    first.speed = 2000;
    first.drive.motionState = CAR_MOTION_STANDING_START;
    first.drive.engineRpm = 3000;
    first.drive.acceleratorInput.value = 256;
    first.drive.standingStartSpin = 5000;
    PlayerCarRuntime second = first;
    second.drive.standingStartSpin = 6000;
    second.drive.brakeInput = 100;
    PlayerCarRuntime alone = first;
    StepCarStandingStart(&first, 3, 7);
    CHECK_EQ(first.drive.standingStartSpin, 4712);
    CHECK_EQ(first.drive.standingStartBounceY, 3);
    CHECK_EQ(first.drive.standingStartBounceX, 7);
    CHECK_EQ(first.speed, 200);
    StepCarStandingStart(&alone, 3, 7);
    for (int i = 0; i < 5; i++) {
        StepCarStandingStart(&second, i + 2, i + 4);
        StepCarStandingStart(&first, i, i + 1);
        StepCarStandingStart(&alone, i, i + 1);
    }
    CHECK_EQ(memcmp(&first, &alone, sizeof(first)), 0);
    PlayerCarRuntime restored = first;
    StepCarStandingStart(&second, 1, 2);
    StepCarStandingStart(&first, 2, 3);
    StepCarStandingStart(&restored, 2, 3);
    CHECK_EQ(memcmp(&first, &restored, sizeof(first)), 0);
    first.drive.standingStartSpin = CAR_STANDING_START_MIN_SPIN - 1;
    StepCarStandingStart(&first, 0x7FFF, 0x7FFF);
    CHECK_EQ(first.drive.motionState, CAR_MOTION_DRIVING);
    CHECK_EQ(first.drive.standingStartBounceY, 0);
    CHECK_EQ(first.drive.standingStartBounceX, 0);

    puts("standing start setup tests passed");
    return 0;
}
