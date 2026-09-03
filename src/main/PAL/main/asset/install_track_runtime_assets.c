#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track_internal.h"
#include "game/track_camera_internal.h"
#include "rage/track_asset_identity.h"

enum {
    TRACK_RUNTIME_RENDER_TABLE = 0,
    TRACK_RUNTIME_ENVIRONMENT_PALETTE = 1,
    TRACK_RUNTIME_ENVIRONMENT_SCRIPT = 2,
    TRACK_RUNTIME_PRIMARY_MODELS = 3,
    TRACK_RUNTIME_POINTS = 4,
    TRACK_RUNTIME_COURSE_MODELS = 5,
    TRACK_RUNTIME_SECONDARY_MODELS = 6,
    TRACK_RUNTIME_TERRAIN_CELLS = 7,
    TRACK_RUNTIME_COURSE_OBJECTS = 8,
    TRACK_RUNTIME_EVENTS = 9,
    TRACK_RUNTIME_CAMERAS = 10,
    TRACK_RUNTIME_BLOCK_COUNT = 11,
    TRACK_RENDER_CAR_MODEL_COUNT = 11,
    COURSE_OBJECT_KNOWN_FLAGS =
        COURSE_OBJECT_ALTERNATE_NORMAL |
        COURSE_OBJECT_ALTERNATE_ENVIRONMENT_4 |
        COURSE_OBJECT_ENVIRONMENT_4 |
        COURSE_OBJECT_BLINK_ENVIRONMENT_4,
};

_Static_assert(TRACK_RUNTIME_BLOCK_COUNT ==
                   sizeof(((GameSceneAssetHeader *)0)->offsets) /
                       sizeof(((GameSceneAssetHeader *)0)->offsets[0]),
               "runtime block names must cover the scene header");

static s32 IsValidCourseObjectTable(const CourseObjectTable *table,
                                    size_t size, s32 modelCount) {
    u32 i;

    if (size < offsetof(CourseObjectTable, objects) ||
        table->count >
            (size - offsetof(CourseObjectTable, objects)) /
                sizeof(table->objects[0])) {
        return 0;
    }
    for (i = 0; i < table->count; i++) {
        const CourseObject *object = &table->objects[i];

        if ((object->modelId != -1 &&
             (object->modelId < 0 || object->modelId >= modelCount)) ||
            (object->flags & ~COURSE_OBJECT_KNOWN_FLAGS) != 0) {
            return 0;
        }
    }
    return 1;
}

static s32 IsTrackRuntimeAssetIndex(s32 assetIndex) {
    const s32 lastAsset = TrackCourseAssetIndex(
        ASSET_TRACK_2ND_BASE, TRACK_CLASS_COUNT - 1,
        TRACK_COURSE_COUNT - 1);

    return assetIndex >= ASSET_TRACK_2ND_BASE && assetIndex <= lastAsset &&
           ((assetIndex - ASSET_TRACK_2ND_BASE) %
            TRACK_ASSETS_PER_COURSE) == 0;
}

s32 InstallTrackRuntimeAssetPack(const void *data, size_t size, s32 assetIndex,
                                 s32 useSeriesCamera) {
    const GameSceneAssetHeader *header;
    const CourseModelAssetHeader *courseModels;
    const CourseObjectTable *courseObjects;
    const void *blocks[TRACK_RUNTIME_BLOCK_COUNT];
    size_t blockSizes[TRACK_RUNTIME_BLOCK_COUNT];
    s32 i;

    if (!IsTrackRuntimeAssetIndex(assetIndex) || data == NULL ||
        size < sizeof(GameSceneAssetHeader) ||
        size > INT32_MAX) {
        return 0;
    }

    header = (const GameSceneAssetHeader *)data;
    for (i = 0; i < TRACK_RUNTIME_BLOCK_COUNT; i++) {
        s32 start = header->offsets[i];
        s32 end = i + 1 < TRACK_RUNTIME_BLOCK_COUNT
                      ? header->offsets[i + 1]
                      : (s32)size;

        if (start < (s32)sizeof(*header) || end <= start ||
            (size_t)end > size) {
            return 0;
        }
        blocks[i] = (const u8 *)data + start;
        blockSizes[i] = (size_t)(end - start);
    }
    courseObjects = blocks[TRACK_RUNTIME_COURSE_OBJECTS];
    courseModels =
        GetCourseModelAssetHeader(blocks[TRACK_RUNTIME_COURSE_MODELS]);
    if (blockSizes[TRACK_RUNTIME_RENDER_TABLE] <
            offsetof(TrackRenderTable, models) +
                TRACK_RENDER_CAR_MODEL_COUNT *
                    sizeof(CarModelRenderParams) ||
        blockSizes[TRACK_RUNTIME_ENVIRONMENT_PALETTE] <
            ENVIRONMENT_PALETTE_COUNT * sizeof(EnvironmentPalette) ||
        blockSizes[TRACK_RUNTIME_COURSE_OBJECTS] <
            offsetof(CourseObjectTable, objects)) {
        return 0;
    }
    if (!IsValidModelBankAsset(
            GetModelBankHeader(blocks[TRACK_RUNTIME_PRIMARY_MODELS]),
            blockSizes[TRACK_RUNTIME_PRIMARY_MODELS]) ||
        !IsValidCourseModelAsset(
            courseModels,
            blockSizes[TRACK_RUNTIME_COURSE_MODELS]) ||
        !IsValidCourseObjectTable(
            courseObjects, blockSizes[TRACK_RUNTIME_COURSE_OBJECTS],
            courseModels->modelCount) ||
        !IsValidModelBankAsset(
            GetModelBankHeader(blocks[TRACK_RUNTIME_SECONDARY_MODELS]),
            blockSizes[TRACK_RUNTIME_SECONDARY_MODELS]) ||
        !IsValidTerrainCellAsset(
            blocks[TRACK_RUNTIME_TERRAIN_CELLS],
            blockSizes[TRACK_RUNTIME_TERRAIN_CELLS]) ||
        !IsValidEnvironmentScript(
            blocks[TRACK_RUNTIME_ENVIRONMENT_SCRIPT],
            blockSizes[TRACK_RUNTIME_ENVIRONMENT_SCRIPT]) ||
        !IsValidTrackPointAsset(
            blocks[TRACK_RUNTIME_POINTS],
            blockSizes[TRACK_RUNTIME_POINTS]) ||
        !IsValidTrackEventAsset(
            blocks[TRACK_RUNTIME_EVENTS],
            blockSizes[TRACK_RUNTIME_EVENTS]) ||
        !IsValidTrackCameraTable(
            blocks[TRACK_RUNTIME_CAMERAS],
            blockSizes[TRACK_RUNTIME_CAMERAS], useSeriesCamera)) {
        return 0;
    }

    if (!SetEnvironmentScript(
            blocks[TRACK_RUNTIME_ENVIRONMENT_SCRIPT],
            blockSizes[TRACK_RUNTIME_ENVIRONMENT_SCRIPT])) {
        return 0;
    }
    if (!RegisterModelBank(
            GetModelBankHeader(blocks[TRACK_RUNTIME_PRIMARY_MODELS]),
            blockSizes[TRACK_RUNTIME_PRIMARY_MODELS], 1)) {
        return 0;
    }
    if (!InstallTrackPoints(
            blocks[TRACK_RUNTIME_POINTS],
            blockSizes[TRACK_RUNTIME_POINTS])) {
        return 0;
    }
    if (!RegisterCourseModels(
            courseModels,
            blockSizes[TRACK_RUNTIME_COURSE_MODELS]) ||
        !RegisterModelBank(
            GetModelBankHeader(blocks[TRACK_RUNTIME_SECONDARY_MODELS]),
            blockSizes[TRACK_RUNTIME_SECONDARY_MODELS], 2) ||
        !InstallTerrainCellData(
            blocks[TRACK_RUNTIME_TERRAIN_CELLS],
            blockSizes[TRACK_RUNTIME_TERRAIN_CELLS])) {
        return 0;
    }
    if (!InstallTrackEventData(
            blocks[TRACK_RUNTIME_EVENTS],
            blockSizes[TRACK_RUNTIME_EVENTS])) {
        return 0;
    }
    if (!SelectTrackCameraTable(
            blocks[TRACK_RUNTIME_CAMERAS],
            blockSizes[TRACK_RUNTIME_CAMERAS], useSeriesCamera)) {
        return 0;
    }
    g_TrackRenderTable = blocks[TRACK_RUNTIME_RENDER_TABLE];
    g_EnvPaletteTable = blocks[TRACK_RUNTIME_ENVIRONMENT_PALETTE];
    g_CourseObjects = courseObjects->objects;
    g_CourseObjectCount = (s32)courseObjects->count;
    TrackAssetIdentitySet(assetIndex);
    return 1;
}
