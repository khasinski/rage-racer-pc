#ifndef GAME_RESULT_SCREEN_TYPES_H
#define GAME_RESULT_SCREEN_TYPES_H

#include "common.h"

typedef struct ResultPlaceBarPosition {
    u8 left;
    u8 right;
} ResultPlaceBarPosition;

typedef struct ResultPlaceSpriteLayout {
    u8 x;
    u8 width;
    u8 u;
} ResultPlaceSpriteLayout;

enum { RESULT_PLACE_COUNT = 3 };

/* These tables retain trailing bytes present in the retail data segment.
 * Keeping the padding in the type makes the symbol sizes exact without
 * pretending it is another drawable entry. */
typedef struct ResultPlaceSpriteTable {
    ResultPlaceSpriteLayout places[RESULT_PLACE_COUNT];
    u8 padding;
} ResultPlaceSpriteTable;

typedef struct ResultPlaceBarTable {
    ResultPlaceBarPosition places[RESULT_PLACE_COUNT];
    u8 padding[2];
} ResultPlaceBarTable;

typedef struct ResultPanelClutTable {
    u16 byPlace[RESULT_PLACE_COUNT + 1];
    u16 padding;
} ResultPanelClutTable;

_Static_assert(sizeof(ResultPlaceSpriteTable) == 10,
               "result place sprite table ABI size changed");
_Static_assert(sizeof(ResultPlaceBarTable) == 8,
               "class place bar table ABI size changed");
_Static_assert(sizeof(ResultPanelClutTable) == 10,
               "result panel CLUT table ABI size changed");

extern ResultPlaceSpriteTable g_ResultPlaceSprites;
extern ResultPlaceBarTable g_ClassPlaceBarSizes;
extern ResultPanelClutTable g_ResultPanelCluts;

#endif
