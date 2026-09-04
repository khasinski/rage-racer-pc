#include "../../src/port/host_state_fmv.c"

_Static_assert(sizeof(g_FmvStreamEnded) == sizeof(s32),
               "FMV stream state must be a scalar");
_Static_assert(sizeof(g_FmvState) == sizeof(s32),
               "FMV state must be a scalar");
_Static_assert(sizeof(g_StreamReturnScene) == sizeof(s32),
               "FMV return scene must be a scalar");
