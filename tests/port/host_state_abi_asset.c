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
