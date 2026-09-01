#include "../../src/port/host_state_menu.c"

_Static_assert(sizeof(g_CarPriceTable) == 128,
               "g_CarPriceTable ABI size changed");
_Static_assert(sizeof(g_CarTuneUpPriceTable) == 124,
               "g_CarTuneUpPriceTable ABI size changed");
