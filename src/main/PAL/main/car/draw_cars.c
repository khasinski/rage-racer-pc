#include "game/car.h"
#include "game/render.h"

enum {
    CAR_MODEL_BANK = 1,
    RIVAL_CAR_COUNT = 11,
};

void DrawCars(void) {
    s32 index;

    SelectModelBank(CAR_MODEL_BANK);
    for (index = 0; index < RIVAL_CAR_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];

        if (car->activeFlag != -1 && car->aiEnabled == 1) {
            DrawCar(GetCarRenderObject(car));
        }
    }
}

void DrawPlayerCarOnly(void) {
    SelectModelBank(CAR_MODEL_BANK);
    DrawCar(GetCarRenderObject(g_Cars));
}
