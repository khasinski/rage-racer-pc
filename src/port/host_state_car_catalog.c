#include "game/car.h"

/* Retail catalog tables shared by the game and catalog/loader regressions.
 * NTSC-U SLUS_004.03: file offsets 0x6c974 and 0x6c984. */
u8 g_CarModelBaseIndex[GAME_CAR_COUNT] __attribute__((aligned(16))) = {
    0, 4, 7, 9, 14, 18, 21, 23, 26, 28, 29, 30, 31
};
u8 g_CarModelUnlockBase[GAME_CAR_COUNT] __attribute__((aligned(16))) = {
    1, 2, 3, 0, 1, 2, 3, 2, 3, 4, 5, 5, 5
};
