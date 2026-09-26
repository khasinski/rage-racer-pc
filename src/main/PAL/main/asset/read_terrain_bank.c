#include "game/terrain_bank.h"
#include "game/model_stream.h"
#include "game/asset_bounds.h"
#include "game/vector.h"
#include <stdint.h>

s32 IsValidTerrainCellAsset(const void *data, size_t size) {
    const u8 *cursor;
    const u16 *grid;
    const TerrainCellAssetHeader *header;
    size_t payloadSize;
    size_t payloadOffset;
    s32 count;
    s32 i;

    if (data == NULL || ((uintptr_t)data & 3u) != 0 || size < TERRAIN_CELL_GRID_BYTES +
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
    const size_t vertices = (payloadSize - (size_t)header->facesOffset) / sizeof(SVec);
    const s32 vertexCount = (s32)(vertices > 65536 ? 65536 : vertices);
    for (i = 0; i < count; i++) {
        if (!AssetPayloadOffsetIsValid(header->cellOffsets[i], payloadOffset,
                                       payloadSize) ||
            !PrimitiveStreamIsValid(cursor, payloadSize, header->cellOffsets[i],
                                    TerrainPrimitiveStride, vertexCount, -1)) {
            return 0;
        }
    }
    return 1;
}

s32 ReadTerrainBank(const void *data, size_t size, TerrainBank *bank) {
    if (!bank || !IsValidTerrainCellAsset(data, size)) return 0;
    const u8 *payload = (const u8 *)data + TERRAIN_CELL_GRID_BYTES + CELL_VISIBILITY_TABLE_SIZE;
    const TerrainCellAssetHeader *header = (const void *)payload;
    TerrainBank view = {.cellCount = header->cellCount,
        .vertices = payload + header->facesOffset, .grid = data,
        .visibility = (const void *)((const u8 *)data + TERRAIN_CELL_GRID_BYTES)};
    for (s32 i = 0; i < header->cellCount; ++i)
        view.cells[i] = payload + header->cellOffsets[i];
    *bank = view;
    return 1;
}
