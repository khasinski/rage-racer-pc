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

    CHECK(g_CarPriceTable[0] == 2600);
    for (index = 0; index < CAR_TUNE_UP_PRICE_COUNT; index++) {
        CHECK(g_CarTuneUpPriceTable[index] == g_CarPriceTable[index + 1]);
    }
    CHECK(g_CarPriceTable[CAR_PRICE_COUNT - 1] == 6666666);
    puts("car and tune-up prices preserve retail's shared value sequence");
    return EXIT_SUCCESS;
}
