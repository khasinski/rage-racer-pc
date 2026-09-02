#ifndef GAME_VISIBILITY_H
#define GAME_VISIBILITY_H

#include "common.h"

enum { TERRAIN_CELL_GRID_SIZE = 32 };

typedef u32 CellVisibilityRow[TERRAIN_CELL_GRID_SIZE];

static inline int CellVisibilityMaskContains(const u32 *mask, s32 column,
                                             s32 row) {
    if ((u32)column >= TERRAIN_CELL_GRID_SIZE ||
        (u32)row >= TERRAIN_CELL_GRID_SIZE) {
        return 0;
    }
    return (mask[row] & (1u << column)) != 0;
}

#endif
