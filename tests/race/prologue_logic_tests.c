#include "game/car.h"
#include "game/race_internal.h"

#include <limits.h>
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
    CHECK(PrologueLineIntensity(INT_MIN) == 0);
    CHECK(PrologueLineIntensity(INT_MAX) == 0);

    CHECK(!IsPrologueWorldActive(-1));
    CHECK(!IsPrologueWorldActive(15));
    CHECK(IsPrologueWorldActive(16));
    CHECK(IsPrologueWorldActive(1054));
    CHECK(!IsPrologueWorldActive(1055));

    CHECK(NextPrologueTimer(-1) == 0);
    CHECK(NextPrologueTimer(1279) == 1280);
    CHECK(NextPrologueTimer(1280) == 1280);
    CHECK(NextPrologueTimer(INT_MAX) == 1280);

    CHECK(AdvancePrologueFade(264, -4, INT_MAX) == 260);
    CHECK(AdvancePrologueFade(2, -4, INT_MAX) == 0);
    CHECK(AdvancePrologueFade(255, 4, 257) == 257);
    CHECK(AdvancePrologueFade(INT_MAX, INT_MAX, 257) == 257);
    CHECK(AdvancePrologueFade(INT_MIN, INT_MIN, 255) == 0);

    CHECK(PrologueCameraIndex(0) == 0);
    CHECK(PrologueCameraIndex(RACE_CAR_SLOT_COUNT - 1) ==
          RACE_CAR_SLOT_COUNT - 1);
    CHECK(PrologueCameraIndex(-1) == 0);
    CHECK(PrologueCameraIndex(RACE_CAR_SLOT_COUNT) == 0);

    puts("prologue logic tests passed");
    return 0;
}
