#include "game/car.h"
#include "game/race.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

GameCarSpec *g_CarSpec;
s16 g_RacePhase;

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

    memset(&spec, 0, sizeof(spec));
    memset(&car, 0, sizeof(car));
    g_CarSpec = &spec;

    g_RacePhase = 1;
    car.tiltCounter = -20;
    UpdatePlayerTilt(&car);
    CHECK(car.tiltCounter == 8);

    g_RacePhase = 2;
    spec.redline = 1000;
    car.verticalMotionState = 0;
    car.drive.engineRpm = 1000;
    car.drive.acceleratorInput.value = 0x81;
    car.drive.clutch = 0;
    car.drive.manual = 1;
    car.tiltCounter = -39;
    UpdatePlayerTilt(&car);
    CHECK(car.tiltCounter == -40);

    car.drive.engineRpm = 0;
    car.speed = 0x51;
    car.tiltCounter = 7;
    car.drive.brakeInput = 0x81;
    UpdatePlayerTilt(&car);
    CHECK(car.tiltCounter == 8);

    car.drive.brakeInput = 0;
    car.drive.clutch = 1;
    car.tiltCounter = 7;
    UpdatePlayerTilt(&car);
    CHECK(car.tiltCounter == 8);

    car.drive.clutch = 0;
    car.tiltCounter = -7;
    UpdatePlayerTilt(&car);
    CHECK(car.tiltCounter == -5);

    car.verticalMotionState = 1;
    car.drive.engineRpm = 1000;
    car.drive.acceleratorInput.value = 0x81;
    car.tiltCounter = 12;
    UpdatePlayerTilt(&car);
    CHECK(car.tiltCounter == 9);

    car.verticalMotionState = CAR_VERTICAL_GROUNDED;
    car.drive.engineRpm = 1000;
    car.drive.acceleratorInput.value = 0x81;
    car.drive.clutch = 0;
    car.tiltCounter = INT16_MIN;
    UpdatePlayerTilt(&car);
    CHECK(car.tiltCounter == INT16_MAX - 3);

    car.drive.engineRpm = 0;
    car.drive.brakeInput = 0x81;
    car.speed = 0x51;
    car.tiltCounter = INT16_MAX;
    UpdatePlayerTilt(&car);
    CHECK(car.tiltCounter == INT16_MIN + 1);

    puts("player tilt tests passed");
    return 0;
}
