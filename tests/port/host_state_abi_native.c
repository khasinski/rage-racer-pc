#include "../../src/port/native_initialized_state.c"

_Static_assert(MEMORY_CARD_SAVE_PATH_STORAGE_SIZE == 80,
               "memory-card path storage ABI changed");
_Static_assert(MEMORY_CARD_SAVE_TITLE_STORAGE_SIZE == 212,
               "memory-card title storage ABI changed");
_Static_assert(sizeof(g_SaveFilePath) == MEMORY_CARD_SAVE_PATH_STORAGE_SIZE,
               "g_SaveFilePath ABI size changed");
_Static_assert(sizeof(g_SaveTitleSjis) == MEMORY_CARD_SAVE_TITLE_STORAGE_SIZE,
               "g_SaveTitleSjis ABI size changed");
_Static_assert(sizeof(g_SaveNameCharset) == 44,
               "save-name charset ABI size changed");
_Static_assert(sizeof(g_PropFontCells) == 128,
               "proportional font table ABI changed");
_Static_assert(sizeof(g_BodyColorPrimary) == 36,
               "primary body-colour table ABI changed");
_Static_assert(sizeof(g_BodyColorSecondary) == 36,
               "secondary body-colour table ABI changed");
_Static_assert(sizeof(g_PaintSlots3StopA) == 20,
               "primary three-stop paint table ABI changed");
_Static_assert(sizeof(g_PaintSlots3StopB) == 16,
               "secondary three-stop paint table ABI changed");
_Static_assert(sizeof(g_PaintSlots4Stop) == 8,
               "four-stop paint table ABI changed");
_Static_assert(sizeof(g_CarModelBankTable) == 44,
               "car model-bank table ABI changed");
_Static_assert(sizeof(g_CarModelByCourse) == 44,
               "course car-model table ABI changed");
_Static_assert(sizeof(g_AtanTable) == 2052,
               "arctangent table ABI changed");
_Static_assert(sizeof(g_RaceGridSlots) == 48,
               "g_RaceGridSlots ABI size changed");
_Static_assert(sizeof(g_CarImageRect) == 8,
               "g_CarImageRect ABI size changed");
_Static_assert(sizeof(g_CountdownDigitPatterns) == 64,
               "countdown digit-pattern table ABI changed");
_Static_assert(sizeof(g_CountdownCellColors) == 16,
               "countdown cell-colour table ABI changed");
_Static_assert(sizeof(g_CountdownCellColors[0]) ==
                   START_COUNTDOWN_COLORS_PER_BANK * sizeof(CVec),
               "countdown colour-bank layout changed");
_Static_assert(sizeof(g_TrackTextureRowRect) == 8,
               "track texture row rectangle ABI changed");
_Static_assert(sizeof(g_TrackColorMatrix) == 32,
               "track colour matrix ABI changed");
_Static_assert(sizeof(g_TrackLightMatrix) == 32,
               "track light matrix ABI changed");
_Static_assert(sizeof(g_DefaultColorMatrix) == 32,
               "default colour matrix ABI changed");
_Static_assert(sizeof(g_DefaultLightMatrix) == 32,
               "default light matrix ABI changed");
_Static_assert(sizeof(g_MenuColorMatrix) == 32,
               "menu colour matrix ABI changed");
_Static_assert(sizeof(g_MenuLightMatrix) == 32,
               "menu light matrix ABI changed");
_Static_assert(sizeof(g_PadButtonPresets) == 128,
               "pad button presets ABI changed");
_Static_assert(sizeof(g_NegconButtonPresets) == 128,
               "NeGcon button presets ABI changed");
_Static_assert(sizeof(g_RaceHudSpriteDescsGp) == 240,
               "g_RaceHudSpriteDescsGp ABI size changed");
_Static_assert(sizeof(g_RaceHudSpriteDescsTimeTrial) == 220,
               "g_RaceHudSpriteDescsTimeTrial ABI size changed");
