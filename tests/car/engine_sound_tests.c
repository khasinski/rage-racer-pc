#include "game/engine_sound.h"
#include <stdio.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
#include <string.h>

int main(void) {
    GameCarSpec specs[2] = {{.revLimit = 9000, .redline = 8500},
                           {.revLimit = 7000, .redline = 6500}};
    GameCarDrive drives[2] = {{.engineRpm = 8000, .gear = 1},
                             {.engineRpm = 0}};
    EngineSound sounds[2] = {{0}}, isolated[2] = {{0}};
    GameCarDrive before[2];
    memcpy(before, drives, sizeof(drives));
    for (u32 frame = 0; frame < 100; ++frame)
        for (unsigned car = 0; car < 2; ++car)
            CHECK(StepEngineSound(&sounds[car], &drives[car], &specs[car], frame, car + 1));
    for (unsigned car = 0; car < 2; ++car)
        for (u32 frame = 0; frame < 100; ++frame)
            CHECK(StepEngineSound(&isolated[car], &drives[car], &specs[car], frame, car + 1));
    CHECK(memcmp(sounds, isolated, sizeof(sounds)) == 0);
    CHECK(memcmp(drives, before, sizeof(drives)) == 0);
    CHECK(sounds[0].rpm > sounds[1].rpm && sounds[1].rpm == 500);
    EngineSound unchanged = sounds[0];
    CHECK(!StepEngineSound(NULL, &drives[0], &specs[0], 0, 1));
    CHECK(!StepEngineSound(&sounds[0], NULL, &specs[0], 0, 1));
    CHECK(!StepEngineSound(&sounds[0], &drives[0], NULL, 0, 1));
    CHECK(memcmp(&sounds[0], &unchanged, sizeof(unchanged)) == 0);
    EngineSound clutch = {0}, engaged = {0};
    drives[0].clutch = 1;
    CHECK(StepEngineSound(&clutch, &drives[0], &specs[0], 0, 1));
    drives[0].clutch = 0;
    CHECK(StepEngineSound(&engaged, &drives[0], &specs[0], 0, 1));
    CHECK(clutch.rpm == 4000 && engaged.rpm == 2000);
    return 0;
}
