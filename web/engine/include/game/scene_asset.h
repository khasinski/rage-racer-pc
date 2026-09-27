#ifndef GAME_SCENE_ASSET_H
#define GAME_SCENE_ASSET_H
#include "common.h"
#include <stddef.h>

enum {
    SCENE_RENDER_TABLE,
    SCENE_ENVIRONMENT_PALETTE,
    SCENE_ENVIRONMENT_SCRIPT,
    SCENE_PRIMARY_MODELS,
    SCENE_POINTS,
    SCENE_COURSE_MODELS,
    SCENE_SECONDARY_MODELS,
    SCENE_TERRAIN_CELLS,
    SCENE_COURSE_OBJECTS,
    SCENE_EVENTS,
    SCENE_CAMERAS,
    SCENE_ASSET_BLOCK_COUNT,
};

typedef struct GameSceneAssetHeader { s32 offsets[SCENE_ASSET_BLOCK_COUNT]; } GameSceneAssetHeader;
typedef struct SceneAssetBlock { const void *data; size_t size; } SceneAssetBlock;
/* Views borrow aligned source storage. Failed validation leaves output intact. */
int ReadSceneAssetBlocks(const void *data, size_t size,
                          SceneAssetBlock blocks[SCENE_ASSET_BLOCK_COUNT]);
/* Owns a copied pack and its block views. Do not copy by value.
 * Validates layout only; consumers still validate typed payloads. */
typedef struct SceneAsset {
    SceneAssetBlock blocks[SCENE_ASSET_BLOCK_COUNT];
    u8 storage[];
} SceneAsset;
SceneAsset *CopySceneAsset(const void *data, size_t size);
void FreeSceneAsset(SceneAsset *asset);
#endif

