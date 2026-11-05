#include "race_mods.h"
#include "rage/menu_animation.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

static int sample, calls;
int RuntimeConfigEnabled(const char *key) {
    return strcmp(key, "mods.random_time_of_day") == 0;
}
int Random15(void) { calls++; return sample; }
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); return 1; } } while (0)
int main(void) {
    CHECK(!PortFastMenusEnabled() && PortMenuAnimationTicks() == 1);
    PortSetFastMenusEnabled(1);
    CHECK(PortFastMenusEnabled() && PortMenuAnimationTicks() == 2);
    CHECK(PortMenuAnimationStep(3) == 6 && PortMenuAnimationStep(-1) == -2);
    CHECK(PortMenuAnimationStep(0) == 0);
    CHECK(PortMenuAnimationStep(INT_MAX) == INT_MAX);
    CHECK(PortMenuAnimationStep(INT_MIN) == INT_MIN);
    PortSetFastMenusEnabled(0);
    CHECK(PortMenuAnimationTicks() == 1 && PortMenuAnimationStep(3) == 3);
    CHECK(PortRandomTimeOfDayEnabled());
    CHECK(PortRaceEnvironmentProfile(3, 0, 5) == 3);
    CHECK(calls == 0);
    CHECK(PortRaceEnvironmentProfile(3, 1, 0) == 3);
    CHECK(calls == 0);
    sample = 0;
    CHECK(PortRaceEnvironmentProfile(3, 1, 5) == 0);
    sample = 16384;
    CHECK(PortRaceEnvironmentProfile(3, 1, 5) == 2);
    sample = 32767;
    CHECK(PortRaceEnvironmentProfile(3, 1, 5) == 4);
    CHECK(PortRaceEnvironmentProfile(0, 1, 3) == 2);
    CHECK(PortRaceEnvironmentProfile(3, 1, 1) == 0);
    PortSetRandomTimeOfDayEnabled(0);
    int before = calls;
    CHECK(PortRaceEnvironmentProfile(3, 1, 5) == 3);
    CHECK(!PortRandomTimeOfDayEnabled() && calls == before);
    puts("GP environment randomization leaves disabled and time-attack paths intact");
    return 0;
}
