#include "common.h"
#include "game/car.h"
#include "game/player_car_internal.h"
#include "game/race.h"

#include <stdio.h>
#include <string.h>

s32 g_LapCount;
GameCarRuntime g_Cars[RACE_CAR_SLOT_COUNT];
PlayerCarRuntime g_PlayerCar;

static s32 s_failures;

static void Check(const char *name, s32 actual, s32 expected) {
    if (actual != expected) {
        printf("FAIL %s: got %d, expected %d\n", name, actual, expected);
        s_failures++;
    }
}

static void Reset(void) {
    s32 car;

    memset(g_Cars, 0, sizeof(g_Cars));
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    for (car = 0; car < RACE_CAR_SLOT_COUNT; car++) {
        g_Cars[car].activeFlag = -1;
    }
    g_LapCount = 3;
    g_PlayerCar.lap = 3;
    g_PlayerCar.progressA = 1000;
    g_PlayerCar.progressB = 200;
    g_PlayerCar.drive.racePosition = 9;
}

int main(void) {
    Reset();
    g_PlayerCar.lap = 4;
    g_Cars[0].activeFlag = 0;
    g_Cars[0].progressA = 9999;
    UpdateRacePosition();
    Check("position is unchanged before the final lap",
          g_PlayerCar.drive.racePosition, 9);

    Reset();
    g_Cars[0].activeFlag = 0;
    g_Cars[0].progressA = 1201;
    g_Cars[1].activeFlag = 0;
    g_Cars[1].progressA = 1300;
    g_Cars[2].activeFlag = 0;
    g_Cars[2].progressA = 1199;
    g_Cars[2].progressB = 1;
    g_Cars[3].activeFlag = 0;
    g_Cars[3].progressA = 1100;
    g_Cars[4].progressA = 9999;
    UpdateRacePosition();
    Check("only active cars strictly ahead count",
          g_PlayerCar.drive.racePosition, 3);

    Reset();
    g_LapCount = 4;
    UpdateRacePosition();
    Check("position updates after reaching the final lap",
          g_PlayerCar.drive.racePosition, 1);

    return s_failures != 0;
}
