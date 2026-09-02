#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/car.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GameRenderState g_RenderState;
NativeModelBank g_ModelBanks[GAME_MODEL_BANK_LIMIT];
NativeCourseModel g_NativeCourseModels[GAME_COURSE_MODEL_LIMIT];
void *g_NativeTerrainCells[GAME_TERRAIN_CELL_LIMIT];
s32 g_ModelBankCount;
s32 g_CourseModelCount;
s32 g_TerrainCellCount;
u16 *g_TerrainCellGrid;
CellVisibilityRow *g_CellVisibilityTable;
u32 g_MainVisibleCellMask[32];
Vec4 g_MainVisibleCellList[64];
u32 *g_VisibleCellMask;
Vec4 *g_VisibleCellList;
s32 g_MirrorMode;

static CarEntry s_cars[2];
CarEntry *g_CarTable = s_cars;
u8 g_CarModelBaseIndex[GAME_CAR_COUNT];
u8 g_CarModelUnlockBase[GAME_CAR_COUNT];
Rect g_CarImageRect;
CarImageData *g_CarImageSlots[CAR_ASSET_SLOT_COUNT];
CarModelAsset *g_CarModelSlots[CAR_ASSET_SLOT_COUNT];
CarModelAsset *g_CarModelAsset;

static s32 s_loadImageCalls;
static RECT *s_loadedRect;
static u_long *s_loadedPixels;

#undef LoadImage
int LoadImage(RECT *rect, u_long *data) {
    s_loadedRect = rect;
    s_loadedPixels = data;
    s_loadImageCalls++;
    return 0;
}

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", \
                __FILE__, __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static int TestModelBank(void) {
    struct {
        u32 modelCount;
        s32 tableOffset;
        s32 normalsOffset;
        s32 modelOffsets[3];
        u8 payload[64];
    } data;
    ModelBankHeader *header = (ModelBankHeader *)&data;

    memset(&data, 0, sizeof(data));
    data.modelCount = 3;
    data.tableOffset = 32;
    data.normalsOffset = 40;
    data.modelOffsets[0] = 48;
    data.modelOffsets[1] = 52;
    data.modelOffsets[2] = 60;
    CHECK(RegisterModelBank(header, sizeof(data), 2) == 1);
    CHECK(g_ModelBanks[2].modelCount == 3);
    CHECK(g_ModelBanks[2].table == (u8 *)&data + 32);
    CHECK(g_ModelBanks[2].normals == (u8 *)&data + 40);
    CHECK(g_ModelBanks[2].models[1] == (u8 *)&data + 52);

    SelectModelBank(2);
    CHECK(g_RenderState.modelTable1 == g_ModelBanks[2].table);
    CHECK(g_RenderState.modelNormals == g_ModelBanks[2].normals);
    CHECK(g_RenderState.modelModels == g_ModelBanks[2].models);
    CHECK(g_ModelBankCount == 3);

    g_ModelBanks[0].modelCount = 77;
    CHECK(RegisterModelBank(header, sizeof(data), -1) == 0);
    CHECK(g_ModelBanks[0].modelCount == 77);
    CHECK(RegisterModelBank(NULL, sizeof(data), 2) == 0);
    CHECK(RegisterModelBank(header,
                            offsetof(ModelBankHeader, modelOffsets), 2) == 0);
    CHECK(g_ModelBanks[2].modelCount == 3);
    data.modelOffsets[1] = sizeof(data);
    CHECK(RegisterModelBank(header, sizeof(data), 2) == 0);
    CHECK(g_ModelBanks[2].modelCount == 3);
    data.modelOffsets[1] = 52;

    {
        size_t size = sizeof(ModelBankHeader) +
                      GAME_MODEL_PER_BANK_LIMIT * sizeof(s32);
        ModelBankHeader *large = calloc(1, size);

        CHECK(large != NULL);
        large->modelCount = UINT32_MAX;
        large->modelOffsets[GAME_MODEL_PER_BANK_LIMIT - 1] = (s32)size - 1;
        CHECK(RegisterModelBank(large, size, 3) == 1);
        CHECK(g_ModelBanks[3].modelCount == GAME_MODEL_PER_BANK_LIMIT);
        CHECK(g_ModelBanks[3].models[GAME_MODEL_PER_BANK_LIMIT - 1] ==
              (u8 *)large + size - 1);
        free(large);
    }
    return 0;
}

static int TestCourseModels(void) {
    struct {
        s32 modelCount;
        CourseModelAssetEntry models[3];
        u8 payload[64];
    } data;
    CourseModelAssetHeader *header = (CourseModelAssetHeader *)&data;

    memset(&data, 0, sizeof(data));
    data.modelCount = 3;
    data.models[0].geometryOffset = 40;
    data.models[0].vertexCount = 12;
    data.models[0].modelOffset = 48;
    data.models[1].geometryOffset = 52;
    data.models[1].vertexCount = 34;
    data.models[1].modelOffset = 60;
    data.models[2].geometryOffset = 64;
    data.models[2].vertexCount = 56;
    data.models[2].modelOffset = 72;
    CHECK(RegisterCourseModels(header, sizeof(data)) == 1);
    CHECK(g_CourseModelCount == 3);
    CHECK(g_RenderState.courseBank == g_NativeCourseModels);
    CHECK(g_NativeCourseModels[1].geometry == (u8 *)&data + 52);
    CHECK(g_NativeCourseModels[1].vertexCount == 34);
    CHECK(g_NativeCourseModels[1].model == (u8 *)&data + 60);

    data.models[1].modelOffset = sizeof(data);
    CHECK(RegisterCourseModels(header, sizeof(data)) == 0);
    CHECK(g_CourseModelCount == 3);
    data.models[1].modelOffset = 60;
    CHECK(RegisterCourseModels(
              header, offsetof(CourseModelAssetHeader, models)) == 0);
    CHECK(g_CourseModelCount == 3);

    data.modelCount = -4;
    CHECK(RegisterCourseModels(header, sizeof(data)) == 1);
    CHECK(g_CourseModelCount == 0);

    {
        size_t size = sizeof(CourseModelAssetHeader) +
                      GAME_COURSE_MODEL_LIMIT *
                          sizeof(CourseModelAssetEntry);
        CourseModelAssetHeader *large = calloc(1, size);

        CHECK(large != NULL);
        large->modelCount = GAME_COURSE_MODEL_LIMIT + 1;
        large->models[GAME_COURSE_MODEL_LIMIT - 1].vertexCount = 99;
        CHECK(RegisterCourseModels(large, size) == 1);
        CHECK(g_CourseModelCount == GAME_COURSE_MODEL_LIMIT);
        CHECK(g_NativeCourseModels[GAME_COURSE_MODEL_LIMIT - 1].vertexCount ==
              99);
        free(large);
    }
    return 0;
}

static int TestTerrainCells(void) {
    enum {
        HEADER_OFFSET = TERRAIN_CELL_GRID_BYTES + CELL_VISIBILITY_TABLE_SIZE,
        BUFFER_SIZE = HEADER_OFFSET + 64,
    };
    u8 data[BUFFER_SIZE];
    TerrainCellAssetHeader *header =
        (TerrainCellAssetHeader *)&data[HEADER_OFFSET];

    memset(data, 0, sizeof(data));
    header->cellCount = 3;
    header->facesOffset = 32;
    header->cellOffsets[0] = 40;
    header->cellOffsets[1] = 44;
    header->cellOffsets[2] = 52;
    CHECK(InstallTerrainCellData(data, sizeof(data)) == 1);
    CHECK(g_TerrainCellGrid == (u16 *)data);
    CHECK(g_CellVisibilityTable ==
          (CellVisibilityRow *)&data[TERRAIN_CELL_GRID_BYTES]);
    CHECK(g_TerrainCellCount == 3);
    CHECK(g_RenderState.cellTable == g_NativeTerrainCells);
    CHECK(g_RenderState.cellFaces == &data[HEADER_OFFSET + 32]);
    CHECK(g_NativeTerrainCells[2] == &data[HEADER_OFFSET + 52]);

    header->cellOffsets[1] = 64;
    CHECK(InstallTerrainCellData(data, sizeof(data)) == 0);
    CHECK(g_TerrainCellCount == 3);
    header->cellOffsets[1] = 44;
    CHECK(InstallTerrainCellData(data, HEADER_OFFSET) == 0);
    CHECK(g_TerrainCellCount == 3);

    header->cellCount = -1;
    CHECK(InstallTerrainCellData(data, sizeof(data)) == 1);
    CHECK(g_TerrainCellCount == 0);

    {
        size_t size = HEADER_OFFSET + sizeof(TerrainCellAssetHeader) +
                      GAME_TERRAIN_CELL_LIMIT * sizeof(s32);
        u8 *large = calloc(1, size);
        TerrainCellAssetHeader *largeHeader;

        CHECK(large != NULL);
        largeHeader = (TerrainCellAssetHeader *)&large[HEADER_OFFSET];
        largeHeader->cellCount = GAME_TERRAIN_CELL_LIMIT + 1;
        largeHeader->cellOffsets[GAME_TERRAIN_CELL_LIMIT - 1] = 1;
        CHECK(InstallTerrainCellData(large, size) == 1);
        CHECK(g_TerrainCellCount == GAME_TERRAIN_CELL_LIMIT);
        CHECK(g_NativeTerrainCells[GAME_TERRAIN_CELL_LIMIT - 1] ==
              (u8 *)largeHeader + 1);
        free(large);
    }
    return 0;
}

static int TestRenderStateAndCarIndexes(void) {
    g_MirrorMode = 7;
    InitRenderState(5);
    CHECK(g_RenderState.faceOtShift == 0xA);
    CHECK(g_RenderState.ft4Color[0] == 0x80);
    CHECK(g_RenderState.ft4Color[3] == POLY_FT4_CODE);
    CHECK(g_RenderState.gt4Color[0] == 0xFF);
    CHECK(g_RenderState.gt4Color[3] == POLY_GT4_CODE);
    CHECK(g_RenderState.x0 == 0 && g_RenderState.y0 == 0);
    CHECK(g_RenderState.x1 == SCREEN_WIDTH &&
          g_RenderState.y1 == SCREEN_HEIGHT);
    CHECK(g_RenderState.otShift == 5 && g_RenderState.orderingFlag == 7);
    CHECK(g_VisibleCellMask == g_MainVisibleCellMask);
    CHECK(g_VisibleCellList == g_MainVisibleCellList);

    g_CarModelBaseIndex[1] = 12;
    g_CarModelUnlockBase[1] = 4;
    g_CarTable[1].modelVariant = 3;
    CHECK(GetCarAssetIndex(1, 2) == 14);
    CHECK(GetCarUnlockLevel(1) == 7);
    return 0;
}

static int TestCarAssetSlots(void) {
    union {
        max_align_t alignment;
        u8 bytes[96];
    } storage;
    SerializedCarModelAssetHeader *serialized =
        GetSerializedCarModelAssetHeader(storage.bytes);
    CarModelAsset *view = GetCarModelAsset(storage.bytes);
    CarModelAsset sentinelModel;
    CarModelAsset unknownModel;
    CarImageData image1;
    size_t completeSize;

    memset(&storage, 0, sizeof(storage));
    view->gearCount = 6;
    view->serializedModelSize = 24;
    serialized->modelOffset = SERIALIZED_CAR_MODEL_HEADER_SIZE;
    serialized->imageOffset = SERIALIZED_CAR_MODEL_HEADER_SIZE + 24;
    completeSize = (size_t)serialized->imageOffset + sizeof(CarImageData);
    g_CarModelSlots[0] = &sentinelModel;
    CHECK(IsValidSerializedCarModelAsset(view, completeSize) == 1);
    CHECK(IsValidSerializedCarModelAsset(view, completeSize - 1) == 0);
    CHECK(InstallSerializedCarModelSlot(view, -1) == 0);
    CHECK(g_CarModelSlots[0] == &sentinelModel);
    CHECK(InstallSerializedCarModelSlot(view, 1) == 1);
    CHECK(g_CarModelSlots[1] != view);
    CHECK(g_CarModelSlots[1]->gearCount == 6);
    CHECK(g_CarModelSlots[1]->serializedModelSize == 24);
    CHECK(g_CarModelSlots[1]->modelData.pointer ==
          storage.bytes + SERIALIZED_CAR_MODEL_HEADER_SIZE);
    CHECK(g_CarModelSlots[1]->imageData.pointer ==
          storage.bytes + SERIALIZED_CAR_MODEL_HEADER_SIZE + 24);
    CHECK(FindSerializedCarModelAsset(g_CarModelSlots[1]) == view);
    CHECK(FindSerializedCarModelAsset(&unknownModel) == NULL);

    CHECK(InstallSerializedCarModelSlot(NULL, 0) == 0);
    serialized->modelOffset++;
    CHECK(InstallSerializedCarModelSlot(view, 0) == 0);
    CHECK(g_CarModelSlots[0] == &sentinelModel);
    serialized->modelOffset = SERIALIZED_CAR_MODEL_HEADER_SIZE;
    serialized->imageOffset++;
    CHECK(InstallSerializedCarModelSlot(view, 0) == 0);
    CHECK(g_CarModelSlots[0] == &sentinelModel);
    serialized->imageOffset = SERIALIZED_CAR_MODEL_HEADER_SIZE + 24;
    view->serializedModelSize = -1;
    CHECK(InstallSerializedCarModelSlot(view, 0) == 0);
    CHECK(g_CarModelSlots[0] == &sentinelModel);

    g_CarModelAsset = &sentinelModel;
    SelectCarModelSlot(2);
    CHECK(g_CarModelAsset == &sentinelModel);
    g_CarModelSlots[0] = NULL;
    SelectCarModelSlot(0);
    CHECK(g_CarModelAsset == &sentinelModel);
    SelectCarModelSlot(1);
    CHECK(g_CarModelAsset == g_CarModelSlots[1]);

    g_CarImageSlots[1] = &image1;
    g_CarImageSlots[0] = NULL;
    s_loadImageCalls = 0;
    UploadCarImage(2);
    CHECK(s_loadImageCalls == 0);
    UploadCarImage(0);
    CHECK(s_loadImageCalls == 0);
    UploadCarImage(1);
    CHECK(s_loadImageCalls == 1 && s_loadedRect == &g_CarImageRect &&
          s_loadedPixels == (u_long *)(void *)&image1);
    return 0;
}

int main(void) {
    if (TestModelBank() != 0) return 1;
    if (TestCourseModels() != 0) return 1;
    if (TestTerrainCells() != 0) return 1;
    if (TestRenderStateAndCarIndexes() != 0) return 1;
    if (TestCarAssetSlots() != 0) return 1;
    puts("model banks, render defaults and car slots retain bounded assets");
    return 0;
}
