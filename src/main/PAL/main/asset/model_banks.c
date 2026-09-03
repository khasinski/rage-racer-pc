#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track.h"

static s32 AssetPayloadOffsetIsValid(s32 offset, size_t payloadOffset,
                                     size_t size) {
    return offset >= 0 && (size_t)offset >= payloadOffset &&
           (size_t)offset < size;
}

s32 IsValidModelBankAsset(const ModelBankHeader *base, size_t size) {
    u32 count;
    u32 i;
    size_t payloadOffset;

    if (base == NULL || size < offsetof(ModelBankHeader, modelOffsets)) {
        return 0;
    }

    if (base->modelCount > GAME_MODEL_PER_BANK_LIMIT) return 0;
    count = base->modelCount;
    payloadOffset = offsetof(ModelBankHeader, modelOffsets) +
                    count * sizeof(base->modelOffsets[0]);
    if (size < payloadOffset ||
        !AssetPayloadOffsetIsValid(base->tableOffset, payloadOffset, size) ||
        !AssetPayloadOffsetIsValid(base->normalsOffset, payloadOffset,
                                   size)) {
        return 0;
    }
    for (i = 0; i < count; i++) {
        if (!AssetPayloadOffsetIsValid(base->modelOffsets[i], payloadOffset,
                                       size)) {
            return 0;
        }
    }
    return 1;
}

s32 RegisterModelBank(ModelBankHeader *base, size_t size, s32 index) {
    NativeModelBank *bank;
    u32 count;
    u32 i;

    if ((u32)index >= GAME_MODEL_BANK_LIMIT ||
        !IsValidModelBankAsset(base, size)) {
        return 0;
    }

    count = base->modelCount;
    bank = &g_ModelBanks[index];
    bank->modelCount = (s32)count;
    bank->table = ResolveAssetAddress(base, base->tableOffset);
    bank->normals = ResolveAssetAddress(base, base->normalsOffset);
    for (i = 0; i < count; i++) {
        bank->models[i] = ResolveAssetAddress(base, base->modelOffsets[i]);
    }
    return 1;
}

void SelectModelBank(s32 index) {
    NativeModelBank *bank;
    if ((u32)index >= GAME_MODEL_BANK_LIMIT) return;
    bank = &g_ModelBanks[index];
    g_RenderState.modelTable1 = bank->table;
    g_RenderState.modelNormals = bank->normals;
    g_ModelBankCount = bank->modelCount;
    g_RenderState.modelModels = bank->models;
}

s32 IsValidCourseModelAsset(const CourseModelAssetHeader *base, size_t size) {
    s32 count;
    s32 i;
    size_t payloadOffset;

    if (base == NULL || size < offsetof(CourseModelAssetHeader, models)) {
        return 0;
    }
    if (base->modelCount < 0 ||
        base->modelCount > GAME_COURSE_MODEL_LIMIT) {
        return 0;
    }
    count = base->modelCount;
    payloadOffset = offsetof(CourseModelAssetHeader, models) +
                    (size_t)count * sizeof(base->models[0]);
    if (size < payloadOffset) {
        return 0;
    }
    for (i = 0; i < count; i++) {
        const CourseModelAssetEntry *entry = &base->models[i];

        if (entry->vertexCount < 0 ||
            !AssetPayloadOffsetIsValid(entry->geometryOffset, payloadOffset,
                                       size) ||
            !AssetPayloadOffsetIsValid(entry->modelOffset, payloadOffset,
                                       size)) {
            return 0;
        }
    }
    return 1;
}

s32 RegisterCourseModels(CourseModelAssetHeader *base, size_t size) {
    s32 count;
    s32 i;

    if (!IsValidCourseModelAsset(base, size)) return 0;

    count = base->modelCount;
    g_RenderState.courseBank = g_NativeCourseModels;
    g_CourseModelCount = count;
    for (i = 0; i < count; i++) {
        const CourseModelAssetEntry *entry = &base->models[i];

        g_NativeCourseModels[i].geometry =
            ResolveAssetAddress(base, entry->geometryOffset);
        g_NativeCourseModels[i].vertexCount = entry->vertexCount;
        g_NativeCourseModels[i].model =
            ResolveAssetAddress(base, entry->modelOffset);
    }
    return 1;
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
    cursor += TERRAIN_CELL_GRID_BYTES;
    cursor += CELL_VISIBILITY_TABLE_SIZE;
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

s32 InstallTerrainCellData(void *data, size_t size) {
    u8 *cursor;
    TerrainCellAssetHeader *header;
    s32 count;
    s32 i;

    if (!IsValidTerrainCellAsset(data, size)) return 0;

    cursor = data;
    cursor += TERRAIN_CELL_GRID_BYTES + CELL_VISIBILITY_TABLE_SIZE;
    header = (TerrainCellAssetHeader *)cursor;
    count = header->cellCount;
    g_TerrainCellGrid = (u16 *)data;
    g_CellVisibilityTable =
        (CellVisibilityRow *)((u8 *)data + TERRAIN_CELL_GRID_BYTES);
    g_RenderState.cellTable = g_NativeTerrainCells;
    g_TerrainCellCount = count;
    g_RenderState.cellFaces = cursor + header->facesOffset;
    for (i = 0; i < count; i++) {
        g_NativeTerrainCells[i] = cursor + header->cellOffsets[i];
    }
    return 1;
}
