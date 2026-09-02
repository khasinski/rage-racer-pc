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
};

s32 InstallTrackRuntimeAssetPack(void *data, size_t size, s32 assetIndex,
                                 s32 useSeriesCamera) {
    GameSceneAssetHeader *header;
    CourseObjectTable *courseObjects;
    size_t blockSizes[11];
    s32 i;

    if (data == NULL || size < sizeof(GameSceneAssetHeader) ||
        size > INT32_MAX) {
        return 0;
    }

    header = GetSceneAssetHeader(data);
    for (i = 0; i < 11; i++) {
        s32 start = header->offsets[i];
        s32 end = i + 1 < 11 ? header->offsets[i + 1] : (s32)size;

        if (start < (s32)sizeof(*header) || end <= start ||
            (size_t)end > size) {
            return 0;
        }
        blockSizes[i] = (size_t)(end - start);
    }
    courseObjects = GetSceneAssetBlock(
        header, TRACK_RUNTIME_COURSE_OBJECTS);
    if (blockSizes[TRACK_RUNTIME_COURSE_OBJECTS] <
            offsetof(CourseObjectTable, objects) ||
        courseObjects->count >
            (blockSizes[TRACK_RUNTIME_COURSE_OBJECTS] -
             offsetof(CourseObjectTable, objects)) /
                sizeof(courseObjects->objects[0])) {
        return 0;
    }
    if (!IsValidModelBankAsset(
            GetModelBankHeader(GetSceneAssetBlock(
                header, TRACK_RUNTIME_PRIMARY_MODELS)),
            blockSizes[TRACK_RUNTIME_PRIMARY_MODELS]) ||
        !IsValidCourseModelAsset(
            GetCourseModelAssetHeader(GetSceneAssetBlock(
                header, TRACK_RUNTIME_COURSE_MODELS)),
            blockSizes[TRACK_RUNTIME_COURSE_MODELS]) ||
        !IsValidModelBankAsset(
            GetModelBankHeader(GetSceneAssetBlock(
                header, TRACK_RUNTIME_SECONDARY_MODELS)),
            blockSizes[TRACK_RUNTIME_SECONDARY_MODELS]) ||
        !IsValidTerrainCellAsset(
            GetSceneAssetBlock(header, TRACK_RUNTIME_TERRAIN_CELLS),
            blockSizes[TRACK_RUNTIME_TERRAIN_CELLS])) {
        return 0;
    }

    TrackAssetIdentitySet(assetIndex);
    g_TrackRenderTable = GetSceneAssetBlock(
        header, TRACK_RUNTIME_RENDER_TABLE);
    g_EnvPaletteTable = GetSceneAssetBlock(
        header, TRACK_RUNTIME_ENVIRONMENT_PALETTE);
    SetEnvironmentScript(GetSceneAssetBlock(
        header, TRACK_RUNTIME_ENVIRONMENT_SCRIPT));
    if (!RegisterModelBank(
            GetModelBankHeader(GetSceneAssetBlock(
                header, TRACK_RUNTIME_PRIMARY_MODELS)),
            blockSizes[TRACK_RUNTIME_PRIMARY_MODELS], 1)) {
        return 0;
    }
    InstallTrackPoints(GetSceneAssetBlock(header, TRACK_RUNTIME_POINTS));
    if (!RegisterCourseModels(
            GetCourseModelAssetHeader(GetSceneAssetBlock(
                header, TRACK_RUNTIME_COURSE_MODELS)),
            blockSizes[TRACK_RUNTIME_COURSE_MODELS]) ||
        !RegisterModelBank(
            GetModelBankHeader(GetSceneAssetBlock(
                header, TRACK_RUNTIME_SECONDARY_MODELS)),
            blockSizes[TRACK_RUNTIME_SECONDARY_MODELS], 2) ||
        !InstallTerrainCellData(
            GetSceneAssetBlock(header, TRACK_RUNTIME_TERRAIN_CELLS),
            blockSizes[TRACK_RUNTIME_TERRAIN_CELLS])) {
        return 0;
    }
    g_CourseObjects = courseObjects->objects;
    g_CourseObjectCount = (s32)courseObjects->count;
    InstallTrackEventData(GetSceneAssetBlock(header, TRACK_RUNTIME_EVENTS));
    SelectTrackCameraTable(GetSceneAssetBlock(header, TRACK_RUNTIME_CAMERAS),
                           useSeriesCamera);
    return 1;
}
