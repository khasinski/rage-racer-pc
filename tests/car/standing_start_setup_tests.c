#include "game/car.h"

#include <stdio.h>
#include <string.h>

s32 g_EngineRpm;
s16 g_PeakOutputRpm;
s16 g_PeakOutputValue;
s16 g_StandingStartState;
s32 g_StandingStartSpin;
s16 g_GripLossTimer;
GameCarSpec *g_CarSpec;

#define CHECK_EQ(actual, expected) do {                                        \
    if ((actual) != (expected)) {                                               \
        fprintf(stderr, "line %d: %s = %d, expected %d\n", __LINE__, #actual, \
                (int)(actual), (int)(expected));                                \
        return 1;                                                               \
    }                                                                           \
} while (0)

int main(void) {
    GameCarSpec spec;
    PlayerCarRuntime car;

    memset(&spec, 0, sizeof(spec));
    memset(&car, 0, sizeof(car));
    g_CarSpec = &spec;
    spec.revLimit = 8000;
    g_EngineRpm = 4000;
    g_PeakOutputRpm = 3000;
    g_PeakOutputValue = 1000;
    car.drive.gear = 2;
    car.drive.drivetrainTorque = 600;
    BeginCarStandingStart(&car);
    CHECK_EQ(car.drive.drivetrainTorque, 300);
    CHECK_EQ(g_StandingStartSpin, 1250);
    CHECK_EQ(g_GripLossTimer, 200);
    CHECK_EQ(g_StandingStartState, 0);

    car.drive.gear = 0;
    car.drive.drivetrainTorque = 600;
    spec.revLimit = 0;
    g_GripLossTimer = 123;
    BeginCarStandingStart(&car);
    CHECK_EQ(car.drive.gear, 1);
    CHECK_EQ(car.drive.drivetrainTorque, 600);
    CHECK_EQ(g_GripLossTimer, 0);

    spec.revLimit = 8000;
    g_EngineRpm = 1500;
    g_PeakOutputRpm = 3000;
    BeginCarStandingStart(&car);
    CHECK_EQ(g_StandingStartSpin, 0);

    g_EngineRpm = 2500;
    BeginCarStandingStart(&car);
    CHECK_EQ(g_StandingStartSpin, 1500);

    car.drive.gear = CAR_FORWARD_GEAR_COUNT + 5;
    car.drive.drivetrainTorque = 600;
    g_EngineRpm = 4000;
    BeginCarStandingStart(&car);
    CHECK_EQ(car.drive.gear, CAR_FORWARD_GEAR_COUNT);
    CHECK_EQ(car.drive.drivetrainTorque, 100);
    CHECK_EQ(g_GripLossTimer, 200);

    puts("standing start setup tests passed");
    return 0;
}
