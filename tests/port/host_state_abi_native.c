#include "../../src/port/native_initialized_state.c"

_Static_assert(sizeof(g_SaveFilePath) == 80,
               "g_SaveFilePath ABI size changed");
_Static_assert(sizeof(g_RaceGridSlots) == 48,
               "g_RaceGridSlots ABI size changed");
_Static_assert(sizeof(g_CarImageRect) == 8,
               "g_CarImageRect ABI size changed");
_Static_assert(sizeof(g_CountdownDigitPatterns) == 64,
               "countdown digit-pattern table ABI changed");
_Static_assert(sizeof(g_CountdownCellColors) == 16,
               "countdown cell-colour table ABI changed");
_Static_assert(sizeof(g_TrackTextureRowRect) == 8,
               "track texture row rectangle ABI changed");
_Static_assert(sizeof(g_RaceHudSpriteDescsGp) == 240,
               "g_RaceHudSpriteDescsGp ABI size changed");
_Static_assert(sizeof(g_RaceHudSpriteDescsTimeTrial) == 220,
               "g_RaceHudSpriteDescsTimeTrial ABI size changed");
