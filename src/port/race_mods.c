#include "race_mods.h"
#include "runtime_config.h"
#include "game/random.h"
#include <stdint.h>

static int s_randomTimeOfDay = -1;

int PortRandomTimeOfDayEnabled(void) {
    if (s_randomTimeOfDay < 0)
        s_randomTimeOfDay = RuntimeConfigEnabled("mods.random_time_of_day") != 0;
    return s_randomTimeOfDay;
}

void PortSetRandomTimeOfDayEnabled(int enabled) {
    s_randomTimeOfDay = enabled != 0;
}

int PortRaceEnvironmentProfile(int original, int grandPrix, int profileCount) {
    int offset;
    if (!grandPrix || profileCount <= 0 || !PortRandomTimeOfDayEnabled())
        return original;
    /* Scale the full 15-bit random result over the available profiles.
     * The caller applies only environment data; class, AI and saves stay intact. */
    offset = (int)((int64_t)Random15() * profileCount / 32768);

    return offset;
}
