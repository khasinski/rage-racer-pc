#include "../../src/port/host_state_render.c"

_Static_assert(sizeof(g_MirrorVisibleCellList) ==
                   64 * sizeof(VisibleTerrainCell),
               "mirror visible-cell list shape changed");
_Static_assert(sizeof(g_MirrorVisibleCellMask) == 32 * sizeof(u32),
               "mirror visible-cell mask shape changed");
_Static_assert(sizeof(g_MirrorViewMatrix) == sizeof(Matrix),
               "mirror view matrix type changed");
_Static_assert(sizeof(g_TrackTexturePageWanted) == sizeof(s32),
               "requested track texture page must be a scalar");
_Static_assert(sizeof(g_SpriteFontWidth) == 96,
               "font widths must not absorb adjacent pointer tables");
_Static_assert(sizeof(g_CameraMatrixSaved) == sizeof(Matrix),
               "saved camera matrix type changed");
_Static_assert(sizeof(g_MenuRowFlashLevels) == 5 * sizeof(s32),
               "menu-row flash table shape changed");
_Static_assert(sizeof(g_TachoNeedleQuad) == 4 * 2 * sizeof(s16),
               "tachometer needle quad shape changed");
_Static_assert(sizeof(g_TrackRenderTable) == sizeof(void *),
               "track render table must use the host pointer width");
_Static_assert(sizeof(g_MirrorPanelY) == sizeof(s32),
               "mirror panel position must be a scalar");
