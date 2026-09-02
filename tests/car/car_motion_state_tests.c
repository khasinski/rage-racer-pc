#include "game/car.h"
#include "game/race.h"

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
    GameCarRuntime car;

    memset(&spec, 0, sizeof(spec));
    memset(&car, 0x5A, sizeof(car));
    g_CarSpec = &spec;
    ClearCarMotionState(&car);
    CHECK(car.collisionFlag == 0 && car.motionMode == 0);
    CHECK(car.motionActive == 0 && car.motionTimer == 0);
    CHECK(car.velocityX == 0 && car.velocityZ == 0);
    CHECK(car.tiltCounter == 0 && car.verticalMotionState == 0);
    CHECK(car.verticalMotionTimer == 0 && car.verticalMotionRate == 0);
    CHECK(car.verticalTargetY == 0);

    g_RacePhase = 1;
    car.tiltCounter = -20;
    UpdateCarTiltCounter(&car);
    CHECK(car.tiltCounter == 8);

    g_RacePhase = 2;
    spec.redline = 1000;
    car.verticalMotionState = 0;
    car.engineRpm = 1000;
    car.acceleratorInput = 0x81;
    car.slideInput.halves.low = 0;
    car.currentGear = 3;
    car.tiltCounter = -29;
    UpdateCarTiltCounter(&car);
    CHECK(car.tiltCounter == -30);

    car.engineRpm = 0;
    car.speed = 0x51;
    car.tiltCounter = 7;
    GetCarAiBlock(&car)->brakeInput = 0x81;
    UpdateCarTiltCounter(&car);
    CHECK(car.tiltCounter == 8);

    GetCarAiBlock(&car)->brakeInput = 0;
    car.tiltCounter = -7;
    UpdateCarTiltCounter(&car);
    CHECK(car.tiltCounter == -5);

    car.verticalMotionState = 1;
    car.engineRpm = 1000;
    car.acceleratorInput = 0x81;
    car.tiltCounter = 12;
    UpdateCarTiltCounter(&car);
    CHECK(car.tiltCounter == 9);

    puts("car motion state tests passed");
    return 0;
}
