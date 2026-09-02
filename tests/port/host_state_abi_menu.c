#include "../../src/port/host_state_menu.c"

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
