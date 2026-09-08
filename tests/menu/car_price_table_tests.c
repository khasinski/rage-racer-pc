#include <stdio.h>
#include <stdlib.h>

#include "game/menu.h"

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return EXIT_FAILURE;                                               \
        }                                                                      \
    } while (0)

int main(void) {
    int index;
    /* Complete retail run, independently verified in SLUS_004.03 at 0x73284.
     * Checking only the overlapping tables lets matching corruption pass. */
    static const s32 expected[CAR_PRICE_COUNT] = {
        2600, 11300, 70500, 361500, 14500, 69400, 329300, 143300,
        583700, 0, 1600, 13200, 61900, 310000, 4000, 10600,
        69900, 362500, 15200, 62400, 331400, 136700, 577000, 20000,
        77500, 405700, 151600, 559700, 695900, 2143500, 2836800, 6666666,
    };
    for (index = 0; index < CAR_PRICE_COUNT; index++) {
        CHECK(g_CarPriceTable[index] == expected[index]);
    }

    CHECK(g_CarPriceTable[0] == 2600);
    for (index = 0; index < CAR_TUNE_UP_PRICE_COUNT; index++) {
        CHECK(g_CarTuneUpPriceTable[index] == g_CarPriceTable[index + 1]);
    }
    CHECK(g_CarPriceTable[CAR_PRICE_COUNT - 1] == 6666666);
    puts("car and tune-up prices preserve retail's shared value sequence");
    return EXIT_SUCCESS;
}
