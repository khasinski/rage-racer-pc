#include <stddef.h>

#include "../../src/port/host_state_fmv.c"

_Static_assert(sizeof(g_Fmv) == 3 * sizeof(s32),
               "FMV runtime must contain exactly its three state words");
_Static_assert(offsetof(Fmv, streamEnded) == 0,
               "FMV stream completion must be the first state word");
_Static_assert(offsetof(Fmv, playback) == sizeof(s32),
               "FMV playback phase must be the second state word");
_Static_assert(offsetof(Fmv, returnScene) == 2 * sizeof(s32),
               "FMV return scene must be the third state word");
