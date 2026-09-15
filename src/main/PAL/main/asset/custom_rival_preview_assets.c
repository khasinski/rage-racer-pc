#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/race.h"
#include "game/track_internal.h"
#include <psyz/gpu.h>

enum {
    CUSTOM_PREVIEW_LOAD_TEXTURES = 1,
    CUSTOM_PREVIEW_LOAD_MODELS,
    CUSTOM_PREVIEW_MODEL_BANK = 13,
};

enum {
    TRACK_PREVIEW_RENDER_TABLE = 0,
    TRACK_PREVIEW_PRIMARY_MODELS = 3,
};

static const TrackRenderTable *s_renderTable;
static u8 *s_buffer;
static size_t s_bufferSize;

const TrackRenderTable *CustomRivalPreviewRenderTable(void) {
    return s_renderTable;
}

static s32 InstallPreviewModels(const void *data, size_t size) {
    const GameSceneAssetHeader *header = data;
    const u8 *bytes = data;
    s32 modelStart;
    s32 modelEnd;

    if (data == NULL || size > INT32_MAX ||
        size < sizeof(GameSceneAssetHeader)) return 0;
    modelStart = header->offsets[TRACK_PREVIEW_PRIMARY_MODELS];
    modelEnd = header->offsets[TRACK_PREVIEW_PRIMARY_MODELS + 1];
    if (header->offsets[TRACK_PREVIEW_RENDER_TABLE] < (s32)sizeof(*header) ||
        modelStart < (s32)sizeof(*header) || modelEnd <= modelStart ||
        (size_t)modelEnd > size) return 0;
    if (!RegisterModelBank(GetModelBankHeader(bytes + modelStart),
                           (size_t)(modelEnd - modelStart),
                           CUSTOM_PREVIEW_MODEL_BANK)) return 0;
    s_renderTable = (const TrackRenderTable *)(const void *)(
        bytes + header->offsets[TRACK_PREVIEW_RENDER_TABLE]);
    return 1;
}

s32 RequestCustomRivalPreviewAssets(void) {
    size_t size;

    s_buffer = CustomPreviewAssetBuffer(&size);
    s_bufferSize = size;
    s_renderTable = NULL;
    if (s_buffer == NULL || size < TRACK_TEXTURE_SHADOW_SIZE) return 0;
    return RestartAssetLoad(ASSET_REQUEST_CUSTOM_RIVAL_PREVIEW,
                            CUSTOM_PREVIEW_LOAD_TEXTURES, 0);
}

void LoadCustomRivalPreviewAssets(void) {
    s32 asset;
    s32 loaded;

    if (g_AssetLoadState == CUSTOM_PREVIEW_LOAD_TEXTURES) {
        asset = TrackCourseAssetIndex(ASSET_TRACK_1ST_BASE,
                                      g_RaceSession.classIndex,
                                      g_RaceSession.course % COURSE_SLOT_COUNT);
        loaded = LoadAsset(asset, s_buffer);
        if (AssetLoadDidNotComplete(loaded)) return;
        Psyz_GpuTextureUploadContext(1);
        s32 installed =
            InstallTrackPreviewTexturePack(s_buffer, (size_t)loaded);
        Psyz_GpuTextureUploadContext(0);
        if (!installed) {
            FailAssetLoad();
            return;
        }
        g_AssetLoadCursor =
            s_buffer + (((size_t)loaded + 15u) & ~(size_t)15u);
        if ((size_t)(g_AssetLoadCursor - s_buffer) >= s_bufferSize) {
            FailAssetLoad();
            return;
        }
        g_AssetLoadState = CUSTOM_PREVIEW_LOAD_MODELS;
        return;
    }
    if (g_AssetLoadState == CUSTOM_PREVIEW_LOAD_MODELS) {
        asset = TrackCourseAssetIndex(ASSET_TRACK_2ND_BASE,
                                      g_RaceSession.classIndex,
                                      g_RaceSession.course % COURSE_SLOT_COUNT);
        loaded = LoadAsset(asset, g_AssetLoadCursor);
        if (AssetLoadDidNotComplete(loaded)) return;
        if (!InstallPreviewModels(g_AssetLoadCursor, (size_t)loaded)) {
            FailAssetLoad();
            return;
        }
        g_AssetLoadState = 0;
        return;
    }
    FailAssetLoad();
}
