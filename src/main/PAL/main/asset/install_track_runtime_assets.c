#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/diagnostics.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track_internal.h"
#include "game/track_camera_internal.h"
#include "rage/track_asset_identity.h"

#include <stdio.h>

enum { TRACK_RENDER_CAR_MODEL_COUNT = 11 };

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
    const CourseModelAssetHeader *courseModels;
    const CourseObjectTable *courseObjects;
    CourseObjects checkedObjects;
    SceneAssetBlock blocks[SCENE_ASSET_BLOCK_COUNT];

    if (!IsTrackRuntimeAssetIndex(assetIndex) || data == NULL ||
        size < sizeof(GameSceneAssetHeader) ||
        size > INT32_MAX) {
        return RejectTrackRuntimePack(assetIndex, "index or header size");
    }

    if (!ReadSceneAssetBlocks(data, size, blocks)) {
        return RejectTrackRuntimePack(assetIndex, "block offsets");
    }
    courseObjects = blocks[SCENE_COURSE_OBJECTS].data;
    courseModels =
        GetCourseModelAssetHeader(blocks[SCENE_COURSE_MODELS].data);
    if (blocks[SCENE_RENDER_TABLE].size <
            offsetof(TrackRenderTable, models) +
                TRACK_RENDER_CAR_MODEL_COUNT *
                    sizeof(CarModelRenderParams) ||
        blocks[SCENE_ENVIRONMENT_PALETTE].size <
            ENVIRONMENT_PALETTE_COUNT * sizeof(EnvironmentPalette) ||
        blocks[SCENE_COURSE_OBJECTS].size <
            offsetof(CourseObjectTable, objects)) {
        return RejectTrackRuntimePack(assetIndex, "fixed block sizes");
    }
    if (!IsValidModelBankAsset(
            GetModelBankHeader(blocks[SCENE_PRIMARY_MODELS].data),
            blocks[SCENE_PRIMARY_MODELS].size)) {
        return RejectTrackRuntimePack(assetIndex, "primary model bank");
    }
    if (!IsValidCourseModelAsset(
            courseModels, blocks[SCENE_COURSE_MODELS].size)) {
        return RejectTrackRuntimePack(assetIndex, "course models");
    }
    if (!ReadCourseObjects(
            courseObjects, blocks[SCENE_COURSE_OBJECTS].size,
            courseModels->modelCount, &checkedObjects)) {
        return RejectTrackRuntimePack(assetIndex, "course object table");
    }
    if (!IsValidModelBankAsset(
            GetModelBankHeader(blocks[SCENE_SECONDARY_MODELS].data),
            blocks[SCENE_SECONDARY_MODELS].size)) {
        return RejectTrackRuntimePack(assetIndex, "secondary model bank");
    }
    if (!IsValidTerrainCellAsset(
            blocks[SCENE_TERRAIN_CELLS].data,
            blocks[SCENE_TERRAIN_CELLS].size)) {
        return RejectTrackRuntimePack(assetIndex, "terrain cells");
    }
    if (!IsValidEnvironmentScript(
            blocks[SCENE_ENVIRONMENT_SCRIPT].data,
            blocks[SCENE_ENVIRONMENT_SCRIPT].size)) {
        return RejectTrackRuntimePack(assetIndex, "environment script");
    }
    if (!IsValidTrackPointAsset(
            blocks[SCENE_POINTS].data,
            blocks[SCENE_POINTS].size)) {
        return RejectTrackRuntimePack(assetIndex, "track points");
    }
    if (!IsValidTrackEventAsset(
            blocks[SCENE_EVENTS].data,
            blocks[SCENE_EVENTS].size)) {
        return RejectTrackRuntimePack(assetIndex, "track events");
    }
    if (!IsValidTrackCameraTable(
            blocks[SCENE_CAMERAS].data,
            blocks[SCENE_CAMERAS].size, useSeriesCamera)) {
        return RejectTrackRuntimePack(assetIndex, "track cameras");
    }

    if (!SetEnvironmentScript(
            blocks[SCENE_ENVIRONMENT_SCRIPT].data,
            blocks[SCENE_ENVIRONMENT_SCRIPT].size)) {
        return RejectTrackRuntimePack(assetIndex, "environment script install");
    }
    if (!RegisterModelBank(
            GetModelBankHeader(blocks[SCENE_PRIMARY_MODELS].data),
            blocks[SCENE_PRIMARY_MODELS].size, 1)) {
        return RejectTrackRuntimePack(assetIndex, "primary model bank install");
    }
    if (!InstallTrackPoints(
            blocks[SCENE_POINTS].data,
            blocks[SCENE_POINTS].size)) {
        return RejectTrackRuntimePack(assetIndex, "track point install");
    }
    if (!RegisterCourseModels(
            courseModels,
            blocks[SCENE_COURSE_MODELS].size) ||
        !RegisterModelBank(
            GetModelBankHeader(blocks[SCENE_SECONDARY_MODELS].data),
            blocks[SCENE_SECONDARY_MODELS].size, 2) ||
        !InstallTerrainCellData(
            blocks[SCENE_TERRAIN_CELLS].data,
            blocks[SCENE_TERRAIN_CELLS].size)) {
        return RejectTrackRuntimePack(assetIndex, "model or terrain install");
    }
    if (!InstallTrackEventData(
            blocks[SCENE_EVENTS].data,
            blocks[SCENE_EVENTS].size)) {
        return RejectTrackRuntimePack(assetIndex, "track event install");
    }
    if (!SelectTrackCameraTable(
            blocks[SCENE_CAMERAS].data,
            blocks[SCENE_CAMERAS].size, useSeriesCamera)) {
        return RejectTrackRuntimePack(assetIndex, "track camera install");
    }
    g_TrackRenderTable = blocks[SCENE_RENDER_TABLE].data;
    g_EnvPaletteTable = blocks[SCENE_ENVIRONMENT_PALETTE].data;
    g_CourseObjects = checkedObjects.items;
    g_CourseObjectCount = (s32)checkedObjects.count;
    TrackAssetIdentitySet(assetIndex);
    return 1;
}
