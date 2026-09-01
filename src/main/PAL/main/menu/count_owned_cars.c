#include "game/car.h"
#include "game/menu.h"

s32 CountOwnedCars(void) {
    s32 count = 0;
    s32 index;

    for (index = 0; index < GAME_CAR_COUNT; index++) {
        if (g_CarTable[index].enabled != 0) {
            count++;
        }
    }
    return count;
}
