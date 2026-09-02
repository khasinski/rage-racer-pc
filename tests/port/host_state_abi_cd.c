#include "../../src/port/host_state_cd.c"

_Static_assert(sizeof(g_CdSearchFile) == 24,
               "g_CdSearchFile ABI size changed");
_Static_assert(sizeof(g_CdCommandPending) == sizeof(s32),
               "pending CD command must be a scalar");
_Static_assert(sizeof(g_CdTrackElapsedLoc) == sizeof(CdlLOC),
               "elapsed CD location type changed");
