#ifndef GAME_VISIBLE_CELL_SCAN_H
#define GAME_VISIBLE_CELL_SCAN_H

#include "common.h"

#define CELL_SCAN_DIRECTION_COUNT 32
#define VISIBLE_CELL_COUNT 64

typedef union CellScanOffsetTable {
    s8 flat[CELL_SCAN_DIRECTION_COUNT * VISIBLE_CELL_COUNT * 2];
    s8 values[CELL_SCAN_DIRECTION_COUNT][VISIBLE_CELL_COUNT][2];
} CellScanOffsetTable;

extern CellScanOffsetTable g_CellScanOffsets;

#endif
