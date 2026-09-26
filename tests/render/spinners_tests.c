#include "game/spinners.h"
#include "game/random.h"
#include <stdio.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    Spinners single = {{0, 64, 128, 256}, {32, 64}}, group = single;
    TickSpinners(&single, 0, 1, 7, 1);
    CHECK(single.angles[0] == 32 && single.angles[1] == 64);
    TickSpinners(&group, 1, 1, 9, 1);
    CHECK(group.angles[0] == 0 && group.angles[1] == 128 && group.angles[3] == 320);
    Spinners first = single, second = group;
    for (u32 i = 2; i < 1100; ++i) {
        TickSpinners(&single, 0, i, 7, 1);
        TickSpinners(&group, 1, i, 9, 1);
    }
    for (u32 i = 2; i < 1100; ++i) TickSpinners(&first, 0, i, 7, 1);
    for (u32 i = 2; i < 1100; ++i) TickSpinners(&second, 1, i, 9, 1);
    CHECK(memcmp(&first, &single, sizeof(single)) == 0);
    CHECK(memcmp(&second, &group, sizeof(group)) == 0);
    const Spinners paused = single;
    TickSpinners(&single, 0, 512, 7, 0);
    CHECK(memcmp(&paused, &single, sizeof(single)) == 0);
    single = (Spinners){{4095, 0, 0, 0}, {32, 64}};
    TickSpinners(&single, 0, 512, 7, 1);
    CHECK(single.angles[0] == 31); /* Old rate applies before refresh. */
    u32 expected = 7 ^ 512;
    CHECK(single.rates[0] == (RandomNext(&expected) & 31));
    CHECK(single.rates[1] == (RandomNext(&expected) & 63));
    TickSpinners(NULL, 0, 1, 0, 1);
    return 0;
}
