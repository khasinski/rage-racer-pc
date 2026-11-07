#include "game/scene_asset.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int ReadSceneAssetBlocks(const void *data, size_t size,
                          SceneAssetBlock blocks[SCENE_ASSET_BLOCK_COUNT]) {
    if (blocks == NULL) return 0;
    if (data == NULL || size < sizeof(GameSceneAssetHeader) || size > INT32_MAX ||
        ((uintptr_t)data & 3u) != 0) return 0;
    GameSceneAssetHeader header;
    memcpy(&header, data, sizeof(header));
    for (s32 i = 0; i < SCENE_ASSET_BLOCK_COUNT; i++) {
        const s32 start = header.offsets[i];
        const s32 end = i + 1 < SCENE_ASSET_BLOCK_COUNT ? header.offsets[i + 1] : (s32)size;
        if (start < (s32)sizeof(header) || end <= start || (size_t)end > size || (start & 3) != 0) return 0;
    }
    for (s32 i = 0; i < SCENE_ASSET_BLOCK_COUNT; i++) {
        const s32 end = i + 1 < SCENE_ASSET_BLOCK_COUNT ? header.offsets[i + 1] : (s32)size;
        blocks[i] = (SceneAssetBlock){.data = (const u8 *)data + header.offsets[i],
                                      .size = (size_t)(end - header.offsets[i])};
    }
    return 1;
}

SceneAsset *CopySceneAsset(const void *data, size_t size) {
    SceneAssetBlock blocks[SCENE_ASSET_BLOCK_COUNT];
    if (!ReadSceneAssetBlocks(data, size, blocks)) return NULL;
    if (size > SIZE_MAX - sizeof(SceneAsset)) return NULL;
    SceneAsset *asset = malloc(sizeof(*asset) + size);
    if (!asset) return NULL;
    memcpy(asset->storage, data, size);
    if (!ReadSceneAssetBlocks(asset->storage, size, asset->blocks)) {
        free(asset);
        return NULL;
    }
    return asset;
}

void FreeSceneAsset(SceneAsset *asset) { free(asset); }
