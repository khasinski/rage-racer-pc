#include "../../src/port/host_state_race.c"

_Static_assert(sizeof(g_BestSectorTimes) == 96,
               "g_BestSectorTimes ABI size changed");
_Static_assert(sizeof(g_BestLapTimes) == 64,
               "g_BestLapTimes ABI size changed");
_Static_assert(sizeof(g_BestTotalTimes) == 64,
               "g_BestTotalTimes ABI size changed");
_Static_assert(sizeof(g_FadeStep) == sizeof(s32),
               "fade step must be a scalar");
_Static_assert(sizeof(g_FrameParity) == sizeof(s32),
               "frame parity must be a scalar");
_Static_assert(sizeof(g_TimeRecordInsertRow) == sizeof(s32),
               "time-record insertion row must be a scalar");
_Static_assert(sizeof(g_CameraCarTrackSection) == sizeof(s16),
               "camera-car track section must be a scalar");
_Static_assert(sizeof(g_ReplayPlayerModelIndex) == sizeof(s16),
               "replay player model index must match the car field");
_Static_assert(sizeof(g_ReplayRivalModelIndex) == sizeof(s16),
               "replay rival model index must match the car field");
