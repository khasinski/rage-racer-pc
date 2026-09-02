#include "game/asset.h"
#include "game/car.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track.h"
#include <string.h>

static s32 ClampAssetCount(s32 count, s32 limit) {
    if (count < 0) return 0;
    if (count > limit) return limit;
    return count;
}

s32 GetCarAssetIndex(s32 model, s32 grade) {
    return g_CarModelBaseIndex[model] + grade;
}

s32 GetCarUnlockLevel(s32 model) {
    return g_CarTable[model].modelVariant + g_CarModelUnlockBase[model];
}

void InitRenderState(s32 otShift) {
    g_RenderState.faceOtShift = 0xA;
    g_RenderState.ft4Color[2] = 0x80;
    g_RenderState.ft4Color[1] = 0x80;
    g_RenderState.ft4Color[0] = 0x80;
    g_RenderState.ft4Color[3] = POLY_FT4_CODE;
    g_RenderState.gt4Color[2] = 0xFF;
    g_RenderState.gt4Color[1] = 0xFF;
    g_RenderState.gt4Color[0] = 0xFF;
    g_RenderState.gt4Color[3] = POLY_GT4_CODE;
    g_RenderState.x1 = SCREEN_WIDTH;
    g_RenderState.y1 = SCREEN_HEIGHT;
    g_VisibleCellMask = g_MainVisibleCellMask;
    g_RenderState.otShift = otShift;
    g_RenderState.x0 = 0;
    g_RenderState.y0 = 0;
    g_VisibleCellList = g_MainVisibleCellList;
    g_RenderState.orderingFlag = g_MirrorMode;
}

void RegisterModelBank(ModelBankHeader *base, s32 index) {
    NativeModelBank *bank;
    u32 i;

    if ((u32)index >= GAME_MODEL_BANK_LIMIT) return;
    bank = &g_ModelBanks[index];
    bank->modelCount = base->modelCount;
    if (bank->modelCount > GAME_MODEL_PER_BANK_LIMIT) {
        bank->modelCount = GAME_MODEL_PER_BANK_LIMIT;
    }
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
    TerrainCellAssetAddress address;
    TerrainCellAssetHeader *header;
    s32 count;
    s32 i;

    address.data = data;
    g_TerrainCellGrid = address.grid;
    address.bytes += TERRAIN_CELL_GRID_SIZE;
    g_CellVisibilityTable = address.visibilityRows;
    address.bytes += CELL_VISIBILITY_TABLE_SIZE;
    header = address.header;
    count = ClampAssetCount(header->cellCount, GAME_TERRAIN_CELL_LIMIT);
    g_RenderState.cellTable = g_NativeTerrainCells;
    g_TerrainCellCount = count;
    g_RenderState.cellFaces = address.bytes + header->facesOffset;
    for (i = 0; i < count; i++) {
        g_NativeTerrainCells[i] = address.bytes + header->cellOffsets[i];
    }
}

void SetCarImageSlot(CarImageData *asset, s32 index) {
    if ((u32)index >= 2) return;
    g_CarImageSlots[index] = asset;
}

void UploadCarImage(s32 index) {
    if ((u32)index >= 2) return;
    LoadImage(&g_CarImageRect, g_CarImageSlots[index]);
}

static CarModelAsset g_NativeCarModelAssets[2];
static CarModelAsset *g_SerializedCarModelAssets[2];

void SetCarModelSlot(CarModelAsset *asset, s32 index) {
    SerializedCarModelAssetHeader *serialized;
    u8 *bytes;

    if ((u32)index >= 2) return;
    serialized = GetSerializedCarModelAssetHeader(asset);
    bytes = GetAssetBytes(serialized);
    memcpy(&g_NativeCarModelAssets[index], serialized->metadata,
           sizeof(serialized->metadata));
    g_NativeCarModelAssets[index].modelData.pointer =
        bytes + serialized->modelOffset;
    g_NativeCarModelAssets[index].imageData.pointer =
        bytes + serialized->imageOffset;
    g_SerializedCarModelAssets[index] = asset;
    g_CarModelSlots[index] = &g_NativeCarModelAssets[index];
}

CarModelAsset *GetSerializedCarModelAsset(CarModelAsset *nativeAsset) {
    u32 i;
    for (i = 0; i < 2; i++) {
        if (nativeAsset == g_CarModelSlots[i])
            return g_SerializedCarModelAssets[i];
    }
    return nativeAsset;
}

void SelectCarModelSlot(s32 index) {
    if ((u32)index >= 2) return;
    g_CarModelAsset = g_CarModelSlots[index];
}
