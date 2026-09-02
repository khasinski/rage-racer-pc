#include "../../src/port/host_state_menu.c"

_Static_assert(sizeof(g_CarPriceTable) == 128,
               "g_CarPriceTable ABI size changed");
_Static_assert(sizeof(g_CarTuneUpPriceTable) == 124,
               "g_CarTuneUpPriceTable ABI size changed");
_Static_assert(sizeof(g_CarSpecBars) == sizeof(s32) * 4,
               "car-spec bar state ABI changed");
_Static_assert(sizeof(g_CourseSelectScrollState) == sizeof(s32),
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
