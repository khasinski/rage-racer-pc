#ifndef GAME_VISIBILITY_H
#define GAME_VISIBILITY_H

#include "common.h"

enum {
    TERRAIN_CELL_GRID_SIZE = 32,
    TERRAIN_CELL_INDEX_MASK = 0x3FF,
    TERRAIN_MISSING_CELL_INDEX = TERRAIN_CELL_INDEX_MASK,
    TERRAIN_CELL_REGION_SHIFT = 10,
    TERRAIN_CELL_REGION_COUNT = 32,
};

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
