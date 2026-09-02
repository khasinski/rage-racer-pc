#include "../../src/port/host_state.c"

_Static_assert(sizeof(g_MirrorVisibleCellList) == 64 * sizeof(Vec4),
               "mirror visible-cell list shape changed");
_Static_assert(sizeof(g_MirrorVisibleCellMask) == 32 * sizeof(u32),
               "mirror visible-cell mask shape changed");
_Static_assert(sizeof(g_MirrorViewMatrix) == sizeof(Matrix),
               "mirror view matrix type changed");
_Static_assert(sizeof(g_FmvState) == sizeof(s32),
               "FMV state must be a scalar");
_Static_assert(sizeof(g_FrontendState) == sizeof(s32),
               "frontend state must be a scalar");
