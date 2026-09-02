#include "../../src/port/host_state_save.c"

_Static_assert(sizeof(g_SaveDefaults) == 104,
               "g_SaveDefaults ABI size changed");
_Static_assert(sizeof(g_McMenuPhase) == sizeof(s32),
               "memory-card prompt ABI changed");
_Static_assert(sizeof(g_McStatusState) == sizeof(s32),
               "memory-card status-state ABI changed");
