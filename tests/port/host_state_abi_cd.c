#include "../../src/port/host_state_cd.c"

_Static_assert(sizeof(g_Cd.search) == 24,
               "g_Cd.search ABI size changed");
_Static_assert(sizeof(g_Cd.pendingCommand) == sizeof(s32),
               "pending CD command must be a scalar");
_Static_assert(sizeof(g_Cd.elapsed) == sizeof(CdlLOC),
               "elapsed CD location type changed");
