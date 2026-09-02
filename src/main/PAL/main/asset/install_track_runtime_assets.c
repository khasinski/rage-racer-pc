#include "game/asset.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track_internal.h"
#include "game/track_camera_internal.h"
#include "rage/track_asset_identity.h"

void InstallTrackRuntimeAssetPack(s32 assetIndex, s32 useSeriesCamera) {
    CourseObjectTable *courseObjects;

    TrackAssetIdentitySet(assetIndex);
    g_TrackRenderTable = SceneAssetBlock(0);
    g_EnvPaletteTable = SceneAssetBlock(1);
    SetEnvironmentScript(SceneAssetBlock(2));
    RegisterModelBank(GetModelBankHeader(SceneAssetBlock(3)), 1);
    InstallTrackPoints(SceneAssetBlock(4));
    RegisterCourseModels(GetCourseModelAssetHeader(SceneAssetBlock(5)));
    RegisterModelBank(GetModelBankHeader(SceneAssetBlock(6)), 2);
    InstallTerrainCellData(SceneAssetBlock(7));
    courseObjects = SceneAssetBlock(8);
    g_CourseObjects = courseObjects->objects;
    g_CourseObjectCount = (s32)courseObjects->count;
    InstallTrackEventData(SceneAssetBlock(9));
    SelectTrackCameraTable(SceneAssetBlock(10), useSeriesCamera);
}
