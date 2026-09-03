#include "game/car.h"
#include "game/menu.h"

#include <stdio.h>
#include <string.h>

static CarEntry s_cars[GAME_CAR_COUNT];
CarEntry *g_CarTable = s_cars;

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__,   \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    CHECK(CountOwnedCars() == 0);

    s_cars[0].enabled = 1;
    s_cars[6].enabled = 2;
    s_cars[GAME_CAR_COUNT - 1].enabled = 0xFF;
    CHECK(CountOwnedCars() == 3);

    memset(s_cars, 1, sizeof(s_cars));
    CHECK(CountOwnedCars() == GAME_CAR_COUNT);

    g_CarTable = NULL;
    CHECK(CountOwnedCars() == 0);

    puts("owned car count tests passed");
    return 0;
}
