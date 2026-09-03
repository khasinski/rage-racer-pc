#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/render.h"
#include "game/track.h"

s32 IsValidTerrainCellAsset(const void *data, size_t size) {
    const u8 *cursor;
    const u16 *grid;
    const TerrainCellAssetHeader *header;
    size_t payloadSize;
    size_t payloadOffset;
    s32 count;
    s32 i;

    if (data == NULL || size < TERRAIN_CELL_GRID_BYTES +
                                  CELL_VISIBILITY_TABLE_SIZE +
                                  offsetof(TerrainCellAssetHeader,
                                           cellOffsets)) {
        return 0;
    }
    grid = data;
    cursor = data;
    cursor += TERRAIN_CELL_GRID_BYTES + CELL_VISIBILITY_TABLE_SIZE;
    header = (const TerrainCellAssetHeader *)cursor;
    payloadSize = size - (size_t)(cursor - (const u8 *)data);
    if (header->cellCount < 0 ||
        header->cellCount > TERRAIN_MISSING_CELL_INDEX) {
        return 0;
    }

    count = header->cellCount;
    for (i = 0; i < TERRAIN_CELL_GRID_SIZE * TERRAIN_CELL_GRID_SIZE; i++) {
        u16 cellIndex = grid[i] & TERRAIN_CELL_INDEX_MASK;

        if (cellIndex != TERRAIN_MISSING_CELL_INDEX && cellIndex >= count) {
            return 0;
        }
    }

    payloadOffset = offsetof(TerrainCellAssetHeader, cellOffsets) +
                    (size_t)count * sizeof(header->cellOffsets[0]);
    if (payloadSize < payloadOffset ||
        !AssetPayloadOffsetIsValid(header->facesOffset, payloadOffset,
                                   payloadSize)) {
        return 0;
    }
    for (i = 0; i < count; i++) {
        if (!AssetPayloadOffsetIsValid(header->cellOffsets[i], payloadOffset,
                                       payloadSize)) {
            return 0;
        }
    }
    return 1;
}

s32 InstallTerrainCellData(const void *data, size_t size) {
    const u8 *cursor;
    const TerrainCellAssetHeader *header;
    s32 count;
    s32 i;

    if (!IsValidTerrainCellAsset(data, size)) return 0;

    cursor = data;
    cursor += TERRAIN_CELL_GRID_BYTES + CELL_VISIBILITY_TABLE_SIZE;
    header = (const TerrainCellAssetHeader *)cursor;
    count = header->cellCount;
    g_TerrainCellGrid = data;
    g_CellVisibilityTable =
        (const CellVisibilityRow *)((const u8 *)data +
                                    TERRAIN_CELL_GRID_BYTES);
    g_RenderState.cellTable = g_NativeTerrainCells;
    g_TerrainCellCount = count;
    g_RenderState.cellFaces = cursor + header->facesOffset;
    for (i = 0; i < count; i++) {
        g_NativeTerrainCells[i] = cursor + header->cellOffsets[i];
    }
    return 1;
}
