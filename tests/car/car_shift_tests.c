#include "game/car_shift.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static PlayerCarRuntime Car(s16 gear, s32 speed, s32 rpm, int manual) {
    PlayerCarRuntime car = {0};
    car.speed = speed;
    car.acceleration = 80;
    car.drive.motionState = CAR_MOTION_DRIVING;
    car.drive.gear = gear;
    car.drive.gearDisp = gear - 1;
    car.drive.engineRpm = rpm;
    car.drive.manual = manual;
    return car;
}

static void Step(PlayerCarRuntime *car, const GameCarSpec *spec, s32 grade) {
    s32 acceleration = 100;
    UpdateCarGearShiftState(car, spec, grade, &acceleration);
    /* The drivetrain publishes the selected gear after the shift stage. */
    car->drive.gearDisp = car->drive.gear;
}

int main(void) {
    GameCarSpec firstSpec = {0}, secondSpec = {0};
    firstSpec.gearRatio[2] = 1000;
    secondSpec.gearRatio[4] = 2500;
    PlayerCarRuntime first = Car(2, 1168, 500, 1);
    PlayerCarRuntime second = Car(4, 2336, 1200, 0);
    PlayerCarRuntime firstReference = first, secondReference = second;

    for (int i = 0; i < 11; ++i) Step(&firstReference, &firstSpec, 0);
    for (int i = 0; i < 11; ++i) Step(&secondReference, &secondSpec, -240);
    for (int i = 0; i < 11; ++i) {
        Step(&first, &firstSpec, 0);
        Step(&second, &secondSpec, -240);
    }
    CHECK(memcmp(&first, &firstReference, sizeof(first)) == 0);
    CHECK(memcmp(&second, &secondReference, sizeof(second)) == 0);
    CHECK(first.drive.shiftTargetSpeed == 1600);
    CHECK(second.drive.shiftTargetSpeed == 1280);
    CHECK(first.drive.clutch == 0 && second.drive.clutch == 0);

    /* An uphill manual shift changes only that car's target. */
    second = Car(4, 2336, 1200, 1);
    Step(&second, &secondSpec, -240);
    CHECK(second.drive.shiftTargetSpeed == 1254);
    CHECK(first.drive.shiftTargetSpeed == 1600);

    /* A copied car contains everything needed to resume a shift. */
    first = Car(2, 1168, 500, 1);
    for (int i = 0; i < 4; ++i) Step(&first, &firstSpec, 0);
    PlayerCarRuntime restored = first;
    for (int i = 0; i < 5; ++i) {
        Step(&first, &firstSpec, 0);
        Step(&second, &secondSpec, -240);
        Step(&restored, &firstSpec, 0);
    }
    CHECK(memcmp(&first, &restored, sizeof(first)) == 0);

    first = Car(2, 1168, 1200, 1);
    second = Car(4, 2336, 600, 0);
    PrepareAirborneDrivetrain(&first, &firstSpec);
    PlayerCarRuntime beforeSecond = first;
    PrepareAirborneDrivetrain(&second, &secondSpec);
    CHECK(memcmp(&first, &beforeSecond, sizeof(first)) == 0);
    CHECK(first.drive.motionState == CAR_MOTION_AIRBORNE);
    CHECK(second.drive.motionState == CAR_MOTION_AIRBORNE);
    CHECK(first.drive.jumpTimer == CAR_AIRBORNE_SHIFT_FRAMES);
    s32 acceleration = 99;
    UpdateCarGearShiftState(&first, &firstSpec, 0, &acceleration);
    CHECK(acceleration == 0 && first.drive.shiftTargetRpm == 1600);
    UpdateCarGearShiftState(&second, &secondSpec, -240, &acceleration);
    CHECK(second.drive.shiftTargetRpm == 1280);
    CHECK(first.drive.shiftTargetRpm == 1600);
    first.drive.gearDisp = first.drive.gear;
    second.drive.gearDisp = second.drive.gear;
    for (int i = 0; i < 19; ++i) {
        Step(&first, &firstSpec, 0);
        Step(&second, &secondSpec, -240);
    }
    CHECK(first.drive.jumpTimer == 0 && first.drive.engineRpm == 1600);
    CHECK(second.drive.jumpTimer == 0 && second.drive.engineRpm == 1280);

    puts("gear shifts are isolated per car and resume from copied state");
    return 0;
}
