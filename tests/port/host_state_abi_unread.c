#include "../../src/port/host_state_unread.c"

_Static_assert(sizeof(g_RouteSceneryPosition) == 16,
               "g_RouteSceneryPosition ABI size changed");
_Static_assert(sizeof(g_AssetPaths) == 540,
               "g_AssetPaths ABI size changed");
