#ifndef GAME_TERRAIN_BANK_H
#define GAME_TERRAIN_BANK_H
#include "game/track.h"
#include <stddef.h>
typedef struct TerrainCellAssetHeader {
    s32 cellCount;
    s32 facesOffset;
    s32 cellOffsets[1];
} TerrainCellAssetHeader;

#define GAME_TERRAIN_CELL_LIMIT 2048
/* Borrows aligned validated source bytes. Failure preserves the destination. */
typedef struct TerrainBank {
    s32 cellCount;
    const void *vertices;
    const void *cells[GAME_TERRAIN_CELL_LIMIT];
    const u16 *grid;
    const CellVisibilityRow *visibility;
} TerrainBank;
s32 IsValidTerrainCellAsset(const void *data, size_t size);
s32 ReadTerrainBank(const void *data, size_t size, TerrainBank *bank);
#endif
