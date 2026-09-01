#include "../../src/port/host_state_race.c"

_Static_assert(sizeof(g_BestSectorTimes) == 96,
               "g_BestSectorTimes ABI size changed");
_Static_assert(sizeof(g_BestLapTimes) == 64,
               "g_BestLapTimes ABI size changed");
_Static_assert(sizeof(g_BestTotalTimes) == 68,
               "g_BestTotalTimes ABI size changed");
