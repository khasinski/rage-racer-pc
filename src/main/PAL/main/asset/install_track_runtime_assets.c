#include "game/asset.h"
#include "game/track_camera_internal.h"
#include "rage/track_asset_identity.h"

void InstallTrackRuntimeAssetPack(s32 assetIndex, s32 useSeriesCamera) {
    TrackAssetIdentitySet(assetIndex);
    SetTrackRenderTable(SceneAssetBlock(0));
    SetEnvPaletteTable(SceneAssetBlock(1));
    SetEnvironmentScript(SceneAssetBlock(2));
    RegisterModelBank(GetModelBankHeader(SceneAssetBlock(3)), 1);
    InstallTrackPoints(SceneAssetBlock(4));
    RegisterCourseModels(GetCourseModelAssetHeader(SceneAssetBlock(5)));
    RegisterModelBank(GetModelBankHeader(SceneAssetBlock(6)), 2);
    InstallTerrainCellData(SceneAssetBlock(7));
    SetCourseObjects(SceneAssetBlock(8));
    InstallTrackEventData(SceneAssetBlock(9));
    SelectTrackCameraTable(SceneAssetBlock(10), useSeriesCamera);
}
