#include "common.h"
#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/car.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/render_workspace.h"
#include "game/track.h"
#include "psyq/gpu.h"
#include "string.h"

/* Kept local: this unit only stores an address into them, while track/ and
 * render/ read them as u32[] rows, Vec4[] entries and plain s32, four
 * incompatible element types across seven files. */

s32 GetCarAssetIndex(s32 model, s32 grade) {
    return g_CarModelBaseIndex[model] + grade;
}

s32 GetCarUnlockLevel(s32 model) {
    return g_CarTable[model].modelVariant + g_CarModelUnlockBase[model];
}

void InitRenderState(s32 otShift) {
    RENDER_FACE_OT_SHIFT = 0xA;
    RENDER_FT4_B = 0x80;
    RENDER_FT4_G = 0x80;
    RENDER_FT4_R = 0x80;
    RENDER_FT4_CODE = POLY_FT4_CODE;
    RENDER_GT4_B = 0xFF;
    RENDER_GT4_G = 0xFF;
    RENDER_GT4_R = 0xFF;
    RENDER_GT4_CODE = POLY_GT4_CODE;
    RENDER_CLIP_X1 = SCREEN_WIDTH;
    RENDER_CLIP_Y1 = SCREEN_HEIGHT;
    g_VisibleCellMask = g_MainVisibleCellMask;
    RENDER_OT_SHIFT = otShift;
    RENDER_CLIP_X0 = 0;
    RENDER_CLIP_Y0 = 0;
    g_VisibleCellList = g_MainVisibleCellList;
    RENDER_MIRROR = g_MirrorMode;
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

void UnrelocateModelBank(ModelBankHeader *base, s32 offset) {
    /* Serialized model banks now remain offset-based at all times. */
    (void)base;
    (void)offset;
}

/*
 * Point the scratchpad model-bank cursor at one registered bank. The bank
 * pointer is re-read from the table before each store because the stores go
 * to the scratchpad, which cse has no reason to believe does not alias it.
 * The two-step address (base into a local, then index it) is what gives
 * retail's base-first addu; `&g_ModelBanks[index]` loses it.
 */
void SelectModelBank(s32 index) {
    NativeModelBank *bank;
    if ((u32)index >= GAME_MODEL_BANK_LIMIT) return;
    bank = &g_ModelBanks[index];
    RENDER_MODEL_TABLE1 = bank->table;
    RENDER_MODEL_NORMALS = bank->normals;
    g_ModelBankCount = bank->modelCount;
    RENDER_MODEL_MODELS = bank->models;
}

void RegisterCourseModels(CourseModelAssetHeader *base) {
    CourseModelAssetEntry *entry;
    s32 count;
    s32 i;
    s32 limit;
    /* The 8-byte frame retail has. Its three sibling loops here get it from
     * being pre-test loops (gcc 2.6.3 spills a dead ST_REGS pseudo after
     * duplicate_loop_exit_test); this one cannot, because every pre-test
     * spelling of a two-pointer loop costs 29 instructions of induction
     * variables. So the frame is asked for directly. */
    s32 pad[2];

    (void)&pad;
    entry = base->models;
    count = base->modelCount;
    RENDER_COURSE_BANK = g_NativeCourseModels;
    if (count > GAME_COURSE_MODEL_LIMIT) count = GAME_COURSE_MODEL_LIMIT;
    g_CourseModelCount = count;
    i = 0;
    if (count > 0) {
        limit = count;
        do {
            g_NativeCourseModels[i].geometry =
                ResolveAssetAddress(base, entry->geometryOffset);
            g_NativeCourseModels[i].vertexCount = entry->vertexCount;
            g_NativeCourseModels[i].model =
                ResolveAssetAddress(base, entry->modelOffset);
            entry++;
            i++;
        } while (i < limit);
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
    count = header->cellCount;
    if (count > GAME_TERRAIN_CELL_LIMIT) count = GAME_TERRAIN_CELL_LIMIT;
    RENDER_CELL_TABLE = g_NativeTerrainCells;
    g_TerrainCellCount = count;
    RENDER_CELL_FACES = address.bytes + header->facesOffset;
    for (i = 0; i < count; i++) {
        g_NativeTerrainCells[i] = address.bytes + header->cellOffsets[i];
    }
}

void SetCarImageSlot(CarImageData *asset, s32 index) {
    g_CarImageSlots[index] = asset;
}

void UploadCarImage(s32 index) {
    LoadImage(&g_CarImageRect, g_CarImageSlots[index]);
}

static CarModelAsset g_NativeCarModelAssets[2];
static CarModelAsset *g_SerializedCarModelAssets[2];

void SetCarModelSlot(CarModelAsset *asset, s32 index) {
    u8 *bytes = (u8 *)asset;
    s32 modelOffset;
    s32 imageOffset;
    if ((u32)index >= 2) return;
    memcpy((u8 *)&g_NativeCarModelAssets[index], bytes, 0x20);
    memcpy((u8 *)&modelOffset, bytes + 0x20, sizeof(modelOffset));
    memcpy((u8 *)&imageOffset, bytes + 0x24, sizeof(imageOffset));
    g_NativeCarModelAssets[index].modelData.pointer = bytes + modelOffset;
    g_NativeCarModelAssets[index].imageData.pointer = bytes + imageOffset;
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
    g_CarModelAsset = g_CarModelSlots[index];
}

void ModelBankNoOp(void) {
}
