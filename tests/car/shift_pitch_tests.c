#include "game/car_drive.h"
#include "game/random.h"
#include <stdio.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    PlayerCarRuntime car = {0};
    GameCarSpec spec = {.revLimit = 8000, .redline = 6000};
    u32 seed = 24884;
    StepCarShiftPitch(&car, &spec, &seed);
    CHECK(seed == 24884 && car.bodyPitch == 0);
    car.drive.shiftRpmDelta = 1;
    car.drive.shiftTargetRpm = 7000;
    StepCarShiftPitch(&car, &spec, &seed);
    CHECK(seed == 24884 && car.bodyPitch == 0);
    car.drive.shiftTargetRpm = 8000;
    StepCarShiftPitch(&car, &spec, &seed);
    CHECK(seed == 24884 && car.bodyPitch == 0);
    car.drive.shiftTargetRpm = 0;
    PlayerCarRuntime restored = car;
    u32 restoredSeed = seed;
    StepCarShiftPitch(&car, &spec, &seed);
    StepCarShiftPitch(&restored, &spec, &restoredSeed);
    CHECK(car.bodyPitch == 70 && seed != 24884);
    CHECK(seed == restoredSeed && memcmp(&car, &restored, sizeof(car)) == 0);
    return 0;
}
