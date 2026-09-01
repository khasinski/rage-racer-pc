#include "game/asset.h"
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
u8 g_CarModelBaseIndex[2];
u8 g_CarModelUnlockBase[2];
Rect g_CarImageRect;
CarImageData *g_CarImageSlots[2];
CarModelAsset *g_CarModelSlots[2];
CarModelAsset *g_CarModelAsset;

#undef LoadImage
int LoadImage(RECT *rect, u_long *data) { (void)rect; (void)data; return 0; }

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
    RegisterModelBank(header, 2);
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
    RegisterModelBank(header, -1);
    CHECK(g_ModelBanks[0].modelCount == 77);

    {
        size_t size = sizeof(ModelBankHeader) +
                      GAME_MODEL_PER_BANK_LIMIT * sizeof(s32);
        ModelBankHeader *large = calloc(1, size);

        CHECK(large != NULL);
        large->modelCount = GAME_MODEL_PER_BANK_LIMIT + 1;
        large->modelOffsets[GAME_MODEL_PER_BANK_LIMIT - 1] = (s32)size - 1;
        RegisterModelBank(large, 3);
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
    RegisterCourseModels(header);
    CHECK(g_CourseModelCount == 3);
    CHECK(g_RenderState.courseBank == g_NativeCourseModels);
    CHECK(g_NativeCourseModels[1].geometry == (u8 *)&data + 52);
    CHECK(g_NativeCourseModels[1].vertexCount == 34);
    CHECK(g_NativeCourseModels[1].model == (u8 *)&data + 60);

    data.modelCount = -4;
    RegisterCourseModels(header);
    CHECK(g_CourseModelCount == 0);

    {
        size_t size = sizeof(CourseModelAssetHeader) +
                      GAME_COURSE_MODEL_LIMIT *
                          sizeof(CourseModelAssetEntry);
        CourseModelAssetHeader *large = calloc(1, size);

        CHECK(large != NULL);
        large->modelCount = GAME_COURSE_MODEL_LIMIT + 1;
        large->models[GAME_COURSE_MODEL_LIMIT - 1].vertexCount = 99;
        RegisterCourseModels(large);
        CHECK(g_CourseModelCount == GAME_COURSE_MODEL_LIMIT);
        CHECK(g_NativeCourseModels[GAME_COURSE_MODEL_LIMIT - 1].vertexCount ==
              99);
        free(large);
    }
    return 0;
}

static int TestTerrainCells(void) {
    enum {
        HEADER_OFFSET = TERRAIN_CELL_GRID_SIZE + CELL_VISIBILITY_TABLE_SIZE,
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
    InstallTerrainCellData(data);
    CHECK(g_TerrainCellGrid == (u16 *)data);
    CHECK(g_CellVisibilityTable ==
          (CellVisibilityRow *)&data[TERRAIN_CELL_GRID_SIZE]);
    CHECK(g_TerrainCellCount == 3);
    CHECK(g_RenderState.cellTable == g_NativeTerrainCells);
    CHECK(g_RenderState.cellFaces == &data[HEADER_OFFSET + 32]);
    CHECK(g_NativeTerrainCells[2] == &data[HEADER_OFFSET + 52]);

    header->cellCount = -1;
    InstallTerrainCellData(data);
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
        InstallTerrainCellData(large);
        CHECK(g_TerrainCellCount == GAME_TERRAIN_CELL_LIMIT);
        CHECK(g_NativeTerrainCells[GAME_TERRAIN_CELL_LIMIT - 1] ==
              (u8 *)largeHeader + 1);
        free(large);
    }
    return 0;
}

int main(void) {
    if (TestModelBank() != 0) return 1;
    if (TestCourseModels() != 0) return 1;
    if (TestTerrainCells() != 0) return 1;
    puts("model_banks: offsets rebased and counts bounded");
    return 0;
}
