#include "../../src/port/host_state_race.c"

_Static_assert(sizeof(g_RefSectorTimes) == 12,
               "reference sector times must retain their retail size");
_Static_assert(sizeof(g_CourseProgress) == 8,
               "course progress selector must remain a host pointer");
_Static_assert(sizeof(g_ClassRecords) == 44,
               "class records must retain all eleven retail entries");
_Static_assert(sizeof(g_CountdownGlyphTable) == 256,
               "countdown glyph table ABI size changed");
_Static_assert(sizeof(g_CountdownGlyphTable[0]) ==
                   START_COUNTDOWN_PATTERN_ROW_COUNT * sizeof(u32),
               "countdown glyph rows changed");
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
_Static_assert(sizeof(g_Replay.playerModel) == sizeof(s16),
               "replay player model index must match the car field");
_Static_assert(sizeof(g_Replay.rivalModel) == sizeof(s16),
               "replay rival model index must match the car field");
