#include "../../src/port/native_initialized_state.c"

_Static_assert(sizeof(g_SaveFilePath) == 80,
               "g_SaveFilePath ABI size changed");
_Static_assert(sizeof(g_RaceGridSlots) == 48,
               "g_RaceGridSlots ABI size changed");
_Static_assert(sizeof(g_RaceHudSpriteDescsGp) == 240,
               "g_RaceHudSpriteDescsGp ABI size changed");
_Static_assert(sizeof(g_RaceHudSpriteDescsTimeTrial) == 220,
               "g_RaceHudSpriteDescsTimeTrial ABI size changed");
