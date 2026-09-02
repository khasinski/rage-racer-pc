#include "game/asset.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track.h"

static s32 ClampAssetCount(s32 count, s32 limit) {
    if (count < 0) return 0;
    if (count > limit) return limit;
    return count;
}

void RegisterModelBank(ModelBankHeader *base, s32 index) {
    NativeModelBank *bank;
    u32 i;

    if ((u32)index >= GAME_MODEL_BANK_LIMIT) return;
    bank = &g_ModelBanks[index];
    bank->modelCount = base->modelCount > GAME_MODEL_PER_BANK_LIMIT
                           ? GAME_MODEL_PER_BANK_LIMIT
                           : (s32)base->modelCount;
    bank->table = ResolveAssetAddress(base, base->tableOffset);
    bank->normals = ResolveAssetAddress(base, base->normalsOffset);
    for (i = 0; i < (u32)bank->modelCount; i++) {
        bank->models[i] = ResolveAssetAddress(base, base->modelOffsets[i]);
    }
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

void RegisterCourseModels(CourseModelAssetHeader *base) {
    s32 count;
    s32 i;

    count = ClampAssetCount(base->modelCount, GAME_COURSE_MODEL_LIMIT);
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
}

void InstallTerrainCellData(void *data) {
    u8 *cursor;
    TerrainCellAssetHeader *header;
    s32 count;
    s32 i;

    cursor = data;
    g_TerrainCellGrid = (u16 *)cursor;
    cursor += TERRAIN_CELL_GRID_BYTES;
    g_CellVisibilityTable = (CellVisibilityRow *)cursor;
    cursor += CELL_VISIBILITY_TABLE_SIZE;
    header = (TerrainCellAssetHeader *)cursor;
    count = ClampAssetCount(header->cellCount, GAME_TERRAIN_CELL_LIMIT);
    g_RenderState.cellTable = g_NativeTerrainCells;
    g_TerrainCellCount = count;
    g_RenderState.cellFaces = cursor + header->facesOffset;
    for (i = 0; i < count; i++) {
        g_NativeTerrainCells[i] = cursor + header->cellOffsets[i];
    }
}
