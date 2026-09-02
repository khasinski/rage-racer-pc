#include "game/car.h"
#include "game/render.h"

enum {
    CAR_MODEL_BANK = 1,
};

void DrawCars(void) {
    s32 index;

    SelectModelBank(CAR_MODEL_BANK);
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];

        if (car->activeFlag != -1 && car->aiEnabled == 1) {
            DrawCar(GetCarRenderObject(car));
        }
    }
}

void DrawReplayRivalCar(void) {
    SelectModelBank(CAR_MODEL_BANK);
    DrawCar(GetCarRenderObject(g_Cars));
}
