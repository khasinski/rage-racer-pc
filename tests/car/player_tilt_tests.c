#include "game/car.h"
#include "game/car_motion_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>


#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    GameCarSpec spec;
    PlayerCarRuntime car;
    int racing = 0;

    memset(&spec, 0, sizeof(spec));
    memset(&car, 0, sizeof(car));

    racing = 0;
    car.tiltCounter = -20;
    UpdateCarTilt(&car, &spec, racing);
    CHECK(car.tiltCounter == 8);

    racing = 1;
    spec.redline = 1000;
    car.verticalMotionState = 0;
    car.drive.engineRpm = 1000;
    car.drive.acceleratorInput.value = 0x81;
    car.drive.clutch = 0;
    car.drive.manual = 1;
    car.tiltCounter = -39;
    UpdateCarTilt(&car, &spec, racing);
    CHECK(car.tiltCounter == -40);

    car.drive.engineRpm = 0;
    car.speed = 0x51;
    car.tiltCounter = 7;
    car.drive.brakeInput = 0x81;
    UpdateCarTilt(&car, &spec, racing);
    CHECK(car.tiltCounter == 8);

    car.drive.brakeInput = 0;
    car.drive.clutch = 1;
    car.tiltCounter = 7;
    UpdateCarTilt(&car, &spec, racing);
    CHECK(car.tiltCounter == 8);

    car.drive.clutch = 0;
    car.tiltCounter = -7;
    UpdateCarTilt(&car, &spec, racing);
    CHECK(car.tiltCounter == -5);

    car.verticalMotionState = 1;
    car.drive.engineRpm = 1000;
    car.drive.acceleratorInput.value = 0x81;
    car.tiltCounter = 12;
    UpdateCarTilt(&car, &spec, racing);
    CHECK(car.tiltCounter == 9);

    car.verticalMotionState = CAR_VERTICAL_GROUNDED;
    car.drive.engineRpm = 1000;
    car.drive.acceleratorInput.value = 0x81;
    car.drive.clutch = 0;
    car.tiltCounter = INT16_MIN;
    UpdateCarTilt(&car, &spec, racing);
    CHECK(car.tiltCounter == INT16_MAX - 3);

    car.drive.engineRpm = 0;
    car.drive.brakeInput = 0x81;
    car.speed = 0x51;
    car.tiltCounter = INT16_MAX;
    UpdateCarTilt(&car, &spec, racing);
    CHECK(car.tiltCounter == INT16_MIN + 1);

    GameCarSpec otherSpec = {0};
    otherSpec.redline = 2000;
    PlayerCarRuntime initial = {0};
    initial.drive.engineRpm = 1500;
    initial.drive.acceleratorInput.value = 256;
    initial.tiltCounter = -20;
    PlayerCarRuntime first = initial;
    PlayerCarRuntime second = initial;
    UpdateCarTilt(&first, &spec, 1);
    UpdateCarTilt(&second, &otherSpec, 1);
    CHECK(first.tiltCounter == -24);
    CHECK(second.tiltCounter == -15);
    PlayerCarRuntime restored = initial;
    UpdateCarTilt(&restored, &spec, 1);
    CHECK(memcmp(&first, &restored, sizeof(first)) == 0);
    UpdateCarTilt(&second, &otherSpec, 0);
    CHECK(second.tiltCounter == 8 && first.tiltCounter == -24);
    first = initial;
    for (int tick = 0; tick < 20; tick++) UpdateCarTilt(&first, &spec, 1);
    CHECK(first.tiltCounter == -45);
    first = initial;
    first.drive.manual = 1;
    for (int tick = 0; tick < 20; tick++) UpdateCarTilt(&first, &spec, 1);
    CHECK(first.tiltCounter == -40);
    puts("player tilt tests passed");

    return 0;
}
