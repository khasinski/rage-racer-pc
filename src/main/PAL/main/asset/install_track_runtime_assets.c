#include "game/asset.h"
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

void InstallTrackRuntimeAssetPack(s32 assetIndex, s32 useSeriesCamera) {
    CourseObjectTable *courseObjects;

    TrackAssetIdentitySet(assetIndex);
    g_TrackRenderTable = SceneAssetBlock(TRACK_RUNTIME_RENDER_TABLE);
    g_EnvPaletteTable = SceneAssetBlock(TRACK_RUNTIME_ENVIRONMENT_PALETTE);
    SetEnvironmentScript(SceneAssetBlock(TRACK_RUNTIME_ENVIRONMENT_SCRIPT));
    RegisterModelBank(
        GetModelBankHeader(SceneAssetBlock(TRACK_RUNTIME_PRIMARY_MODELS)), 1);
    InstallTrackPoints(SceneAssetBlock(TRACK_RUNTIME_POINTS));
    RegisterCourseModels(GetCourseModelAssetHeader(
        SceneAssetBlock(TRACK_RUNTIME_COURSE_MODELS)));
    RegisterModelBank(
        GetModelBankHeader(SceneAssetBlock(TRACK_RUNTIME_SECONDARY_MODELS)), 2);
    InstallTerrainCellData(SceneAssetBlock(TRACK_RUNTIME_TERRAIN_CELLS));
    courseObjects = SceneAssetBlock(TRACK_RUNTIME_COURSE_OBJECTS);
    g_CourseObjects = courseObjects->objects;
    g_CourseObjectCount = (s32)courseObjects->count;
    InstallTrackEventData(SceneAssetBlock(TRACK_RUNTIME_EVENTS));
    SelectTrackCameraTable(SceneAssetBlock(TRACK_RUNTIME_CAMERAS),
                           useSeriesCamera);
}
