#include "../../src/port/host_state_asset.c"

_Static_assert(sizeof(g_AssetRequestType) == 8,
               "g_AssetRequestType ABI size changed");
_Static_assert(sizeof(g_TrackTextureRect) == 8,
               "g_TrackTextureRect ABI size changed");
