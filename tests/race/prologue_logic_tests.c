#include "game/race_internal.h"

#include <stdio.h>

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__,   \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    CHECK(PrologueLineIntensity(32) == 0);
    CHECK(PrologueLineIntensity(33) == 1);
    CHECK(PrologueLineIntensity(95) == 125);
    CHECK(PrologueLineIntensity(96) == 127);
    CHECK(PrologueLineIntensity(144) == 127);
    CHECK(PrologueLineIntensity(145) == 125);
    CHECK(PrologueLineIntensity(207) == 1);
    CHECK(PrologueLineIntensity(208) == 0);

    CHECK(!IsPrologueWorldActive(-1));
    CHECK(!IsPrologueWorldActive(15));
    CHECK(IsPrologueWorldActive(16));
    CHECK(IsPrologueWorldActive(1054));
    CHECK(!IsPrologueWorldActive(1055));

    puts("prologue logic tests passed");
    return 0;
}
