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
_Static_assert(sizeof(g_TrackCameras) == sizeof(void *),
               "g_TrackCameras must be one pointer");
_Static_assert(sizeof(g_TrackArcCenters) == sizeof(void *),
               "g_TrackArcCenters must be one pointer");
_Static_assert(sizeof(g_PathSceneryPosKeys) == sizeof(void *),
               "g_PathSceneryPosKeys must be one pointer");
_Static_assert(sizeof(g_PathSceneryRotKeys) == sizeof(void *),
               "g_PathSceneryRotKeys must be one pointer");
_Static_assert(sizeof(g_EnvScriptCues) == sizeof(void *),
               "g_EnvScriptCues must be one pointer");
_Static_assert(sizeof(g_CourseObjects) == sizeof(void *),
               "g_CourseObjects must be one pointer");
_Static_assert(sizeof(g_EnvPaletteTable) == sizeof(void *),
               "g_EnvPaletteTable must be one pointer");
_Static_assert(sizeof(g_CellVisibilityTable) == sizeof(void *),
               "g_CellVisibilityTable must be one pointer");
_Static_assert(sizeof(g_VisibleCellList) == sizeof(void *),
               "g_VisibleCellList must be one pointer");
_Static_assert(sizeof(g_TerrainCellGrid) == sizeof(void *),
               "g_TerrainCellGrid must be one pointer");
_Static_assert(sizeof(g_VisibleCellMask) == sizeof(void *),
               "g_VisibleCellMask must be one pointer");
_Static_assert(sizeof(g_RouteSceneryKeyframe) == sizeof(void *),
               "g_RouteSceneryKeyframe must be one pointer");
