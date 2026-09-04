#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/diagnostics.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track_internal.h"
#include "game/track_camera_internal.h"
#include "rage/track_asset_identity.h"

#include <stdio.h>

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

/* Every check below returns through here, so a pack the game refuses can
 * say which check refused it when the asset trace is on. */
static s32 RejectTrackRuntimePack(s32 assetIndex, const char *reason) {
    if (DiagnosticsEnabled("asset_trace")) {
        fprintf(stderr, "rage-port: track runtime asset %d rejected: %s\n",
                assetIndex, reason);
    }
    return 0;
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
        return RejectTrackRuntimePack(assetIndex, "index or header size");
    }

    header = (const GameSceneAssetHeader *)data;
    for (i = 0; i < TRACK_RUNTIME_BLOCK_COUNT; i++) {
        s32 start = header->offsets[i];
        s32 end = i + 1 < TRACK_RUNTIME_BLOCK_COUNT
                      ? header->offsets[i + 1]
                      : (s32)size;

        if (start < (s32)sizeof(*header) || end <= start ||
            (size_t)end > size) {
            return RejectTrackRuntimePack(assetIndex, "block offsets");
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
        return RejectTrackRuntimePack(assetIndex, "fixed block sizes");
    }
    if (!IsValidModelBankAsset(
            GetModelBankHeader(blocks[TRACK_RUNTIME_PRIMARY_MODELS]),
            blockSizes[TRACK_RUNTIME_PRIMARY_MODELS])) {
        return RejectTrackRuntimePack(assetIndex, "primary model bank");
    }
    if (!IsValidCourseModelAsset(
            courseModels, blockSizes[TRACK_RUNTIME_COURSE_MODELS])) {
        return RejectTrackRuntimePack(assetIndex, "course models");
    }
    if (!IsValidCourseObjectTable(
            courseObjects, blockSizes[TRACK_RUNTIME_COURSE_OBJECTS],
            courseModels->modelCount)) {
        return RejectTrackRuntimePack(assetIndex, "course object table");
    }
    if (!IsValidModelBankAsset(
            GetModelBankHeader(blocks[TRACK_RUNTIME_SECONDARY_MODELS]),
            blockSizes[TRACK_RUNTIME_SECONDARY_MODELS])) {
        return RejectTrackRuntimePack(assetIndex, "secondary model bank");
    }
    if (!IsValidTerrainCellAsset(
            blocks[TRACK_RUNTIME_TERRAIN_CELLS],
            blockSizes[TRACK_RUNTIME_TERRAIN_CELLS])) {
        return RejectTrackRuntimePack(assetIndex, "terrain cells");
    }
    if (!IsValidEnvironmentScript(
            blocks[TRACK_RUNTIME_ENVIRONMENT_SCRIPT],
            blockSizes[TRACK_RUNTIME_ENVIRONMENT_SCRIPT])) {
        return RejectTrackRuntimePack(assetIndex, "environment script");
    }
    if (!IsValidTrackPointAsset(
            blocks[TRACK_RUNTIME_POINTS],
            blockSizes[TRACK_RUNTIME_POINTS])) {
        return RejectTrackRuntimePack(assetIndex, "track points");
    }
    if (!IsValidTrackEventAsset(
            blocks[TRACK_RUNTIME_EVENTS],
            blockSizes[TRACK_RUNTIME_EVENTS])) {
        return RejectTrackRuntimePack(assetIndex, "track events");
    }
    if (!IsValidTrackCameraTable(
            blocks[TRACK_RUNTIME_CAMERAS],
            blockSizes[TRACK_RUNTIME_CAMERAS], useSeriesCamera)) {
        return RejectTrackRuntimePack(assetIndex, "track cameras");
    }

    if (!SetEnvironmentScript(
            blocks[TRACK_RUNTIME_ENVIRONMENT_SCRIPT],
            blockSizes[TRACK_RUNTIME_ENVIRONMENT_SCRIPT])) {
        return RejectTrackRuntimePack(assetIndex, "environment script install");
    }
    if (!RegisterModelBank(
            GetModelBankHeader(blocks[TRACK_RUNTIME_PRIMARY_MODELS]),
            blockSizes[TRACK_RUNTIME_PRIMARY_MODELS], 1)) {
        return RejectTrackRuntimePack(assetIndex, "primary model bank install");
    }
    if (!InstallTrackPoints(
            blocks[TRACK_RUNTIME_POINTS],
            blockSizes[TRACK_RUNTIME_POINTS])) {
        return RejectTrackRuntimePack(assetIndex, "track point install");
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
        return RejectTrackRuntimePack(assetIndex, "model or terrain install");
    }
    if (!InstallTrackEventData(
            blocks[TRACK_RUNTIME_EVENTS],
            blockSizes[TRACK_RUNTIME_EVENTS])) {
        return RejectTrackRuntimePack(assetIndex, "track event install");
    }
    if (!SelectTrackCameraTable(
            blocks[TRACK_RUNTIME_CAMERAS],
            blockSizes[TRACK_RUNTIME_CAMERAS], useSeriesCamera)) {
        return RejectTrackRuntimePack(assetIndex, "track camera install");
    }
    g_TrackRenderTable = blocks[TRACK_RUNTIME_RENDER_TABLE];
    g_EnvPaletteTable = blocks[TRACK_RUNTIME_ENVIRONMENT_PALETTE];
    g_CourseObjects = courseObjects->objects;
    g_CourseObjectCount = (s32)courseObjects->count;
    TrackAssetIdentitySet(assetIndex);
    return 1;
}
