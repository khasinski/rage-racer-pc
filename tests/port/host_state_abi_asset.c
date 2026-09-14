#include "../../src/port/host_state_asset.c"

_Static_assert(sizeof(g_AssetRequestType) == sizeof(s32),
               "g_AssetRequestType ABI size changed");
_Static_assert(sizeof(g_LoadBuffer) == 1037896,
               "g_LoadBuffer extent changed");
_Static_assert(sizeof(g_CarImageSlots) / sizeof(g_CarImageSlots[0]) == 2,
               "car image slot count changed");
_Static_assert(sizeof(g_AssetBlockPtr) == sizeof(void *),
               "asset pointer storage must use the host pointer width");
_Static_assert(sizeof(g_TrackTextureRect) == 8,
               "g_TrackTextureRect ABI size changed");
_Static_assert(sizeof(g_TeamLogoClutLoadRect) == 8,
               "team-logo CLUT load rectangle ABI changed");
_Static_assert(sizeof(g_TeamLogoClutMoveRect) == 8,
               "team-logo CLUT move rectangle ABI changed");
_Static_assert(sizeof(g_CarModelBaseIndex) == GAME_CAR_COUNT,
               "car model base-index table size changed");
_Static_assert(sizeof(g_CarModelUnlockBase) == GAME_CAR_COUNT,
               "car model unlock table size changed");
