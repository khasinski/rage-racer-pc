#include "../../src/port/host_state_track.c"

_Static_assert(sizeof(g_ShuttlePathPoints) == 96,
               "g_ShuttlePathPoints ABI size changed");
_Static_assert(sizeof(g_EnvironmentColors) == 112,
               "g_EnvironmentColors ABI size changed");
