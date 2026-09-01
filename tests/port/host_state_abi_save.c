#include "../../src/port/host_state_save.c"

_Static_assert(sizeof(g_SaveDefaults) == 104,
               "g_SaveDefaults ABI size changed");
