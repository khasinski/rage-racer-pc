#include "../../src/port/host_state_track.c"

_Static_assert(sizeof(g_ShuttlePathPoints) == 96,
               "g_ShuttlePathPoints ABI size changed");
_Static_assert(sizeof(g_EnvironmentColors) == 112,
               "g_EnvironmentColors ABI size changed");
_Static_assert(sizeof(g_CamPathOffsetDelta) == 12,
               "g_CamPathOffsetDelta ABI size changed");
_Static_assert(sizeof(g_CamPathOffsetStart) == 12,
               "g_CamPathOffsetStart ABI size changed");
_Static_assert(sizeof(g_CamPathOffset) == 12,
               "g_CamPathOffset ABI size changed");
_Static_assert(sizeof(g_CamPathAngleDelta) == 16,
               "g_CamPathAngleDelta ABI size changed");
_Static_assert(sizeof(g_CamPathAngleStart) == 16,
               "g_CamPathAngleStart ABI size changed");
_Static_assert(sizeof(g_CamPathAngle) == 16,
               "g_CamPathAngle ABI size changed");
