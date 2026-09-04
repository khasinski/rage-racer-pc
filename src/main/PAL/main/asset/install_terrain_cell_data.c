#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/render.h"
#include "game/terrain_internal.h"
#include "game/track.h"

static s32 TerrainCellStreamIsValid(const u8 *payload, size_t size,
                                    s32 offset) {
    size_t cursor = (size_t)offset;

    while (cursor <= size && size - cursor >= sizeof(u32)) {
        const u16 primitive = (u16)(payload[cursor] |
                                    (u16)payload[cursor + 1] << 8);
        const u16 count = (u16)(payload[cursor + 2] |
                                (u16)payload[cursor + 3] << 8);
        const s32 stride = TerrainPrimitiveStride(primitive);

        cursor += sizeof(u32);
        if (count == 0) return 1;
        if (stride == 0 || count > (size - cursor) / (size_t)stride) {
            return 0;
        }
        cursor += (size_t)count * (size_t)stride;
    }
    return 0;
}

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
                                       payloadSize) ||
            !TerrainCellStreamIsValid(cursor, payloadSize,
                                      header->cellOffsets[i])) {
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
