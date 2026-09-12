#include <stddef.h>

#include "../../src/port/host_state_fmv.c"

_Static_assert(sizeof(g_FmvRuntime) == 3 * sizeof(s32),
               "FMV runtime must contain exactly its three state words");
_Static_assert(offsetof(FmvRuntimeState, streamEnded) == 0,
               "FMV stream completion must be the first state word");
_Static_assert(offsetof(FmvRuntimeState, playback) == sizeof(s32),
               "FMV playback phase must be the second state word");
_Static_assert(offsetof(FmvRuntimeState, returnScene) == 2 * sizeof(s32),
               "FMV return scene must be the third state word");
