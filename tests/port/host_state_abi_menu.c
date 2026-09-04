#include "../../src/port/host_state_menu.c"

_Static_assert(sizeof(g_FrontendState) == sizeof(s32),
               "frontend state must be a scalar");
_Static_assert(sizeof(g_MsgNegconMaxTwist) == sizeof("Maximum twist."),
               "NeGcon caption must not absorb adjacent retail data");
_Static_assert(sizeof(g_CaptionBestLapTime) == sizeof("hfdi"),
               "lap-time caption must not absorb the BGM name table");
_Static_assert(sizeof(g_CarPriceTable) == 128,
               "g_CarPriceTable ABI size changed");
_Static_assert(sizeof(g_CarTuneUpPriceTable) == 124,
               "g_CarTuneUpPriceTable ABI size changed");
_Static_assert(sizeof(g_CarSpecBars) == sizeof(s32) * 4,
               "car-spec bar state ABI changed");
_Static_assert(sizeof(g_CourseSelectScrollProgress) == sizeof(s32),
               "course-select scroll state ABI changed");
_Static_assert(sizeof(GameMenuCursor) == sizeof(s32),
               "menu cursor state ABI changed");
_Static_assert(sizeof(GameMenuBusy) == sizeof(s32),
               "menu busy state ABI changed");
_Static_assert(sizeof(g_CarModelAsset) == sizeof(void *),
               "car-model pointer ABI changed");
_Static_assert(sizeof(g_CarTable) == sizeof(void *),
               "car-table pointer ABI changed");
_Static_assert(sizeof(g_TeamLogoSampleData) == sizeof(void *),
               "team-logo sample pointer ABI changed");
_Static_assert(sizeof(g_RaceProgress) == sizeof(void *),
               "race-progress pointer ABI changed");
_Static_assert(sizeof(g_TeamLogoClut) == sizeof(u16) * 16,
               "team-logo CLUT ABI changed");
_Static_assert(sizeof(g_TeamLogoCanvas) == 0x800,
               "team-logo canvas ABI changed");
_Static_assert(sizeof(g_MenuCarPivotOffset) == 16,
               "menu car pivot ABI changed");
_Static_assert(sizeof(g_TeamNameCharScale) == 16,
               "team-name character scale ABI changed");
_Static_assert(sizeof(g_TeamNameFontGlyphs) == 84 * 32,
               "team-name glyph atlas ABI changed");
_Static_assert(sizeof(g_TeamNameBlankTile) == 12 * 8 * sizeof(u16),
               "blank team-name texture ABI changed");
_Static_assert(sizeof(g_FormatDecimal) == 4,
               "decimal format size changed");
_Static_assert(sizeof(g_TimeAttackPlateProgress) == sizeof(s32),
               "time-attack plate progress ABI changed");
_Static_assert(sizeof(g_TeamLogoClutRect) == 8,
               "team-logo CLUT rectangle ABI changed");
_Static_assert(sizeof(g_TeamLogoRect) == 8,
               "team-logo rectangle ABI changed");
_Static_assert(sizeof(g_OptionHintCaptions) == 28,
               "option hint caption ABI changed");
_Static_assert(sizeof(g_ClassRecordCellPoints) == 44,
               "class record point ABI changed");
_Static_assert(sizeof(g_ClassRecordCellSprites) == 132,
               "class record sprite ABI changed");
_Static_assert(sizeof(g_ClassRecordNameSprites) == 36,
               "class record name-colour ABI changed");
_Static_assert(sizeof(g_MenuViewScale) == 16,
               "menu view scale ABI changed");
_Static_assert(sizeof(g_MenuLightBurstBandX) == 66,
               "menu light-burst band ABI changed");
_Static_assert(sizeof(g_PaintColorTable) == 54,
               "paint colour table ABI changed");
_Static_assert(sizeof(g_CourseCardVerts) == 32,
               "course-card vertices ABI changed");
_Static_assert(sizeof(g_DesignModeCellMask) == 36,
               "design-mode mask ABI changed");
_Static_assert(sizeof(g_TeamLogoCursorX) == sizeof(TeamLogoCoordinate),
               "team-logo cursor coordinate ABI changed");
_Static_assert(sizeof(g_TeamLogoViewX) == sizeof(TeamLogoCoordinate),
               "team-logo view coordinate ABI changed");
_Static_assert(sizeof(g_TeamLogoPenColor) == sizeof(TeamLogoColorIndex),
               "team-logo pen colour ABI changed");
_Static_assert(sizeof(g_TeamLogoBlankClut) == sizeof(u16) * 16,
               "blank team-logo CLUT ABI changed");
_Static_assert(sizeof(g_TeamLogoFadedClut) == sizeof(u16) * 16,
               "faded team-logo CLUT ABI changed");
_Static_assert(sizeof(g_TeamLogoSwatches) == sizeof(u16) * 15,
               "team-logo swatch ABI changed");
