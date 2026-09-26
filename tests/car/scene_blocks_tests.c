#include "game/scene_asset.h"
#include <stdio.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    u32 words[SCENE_ASSET_BLOCK_COUNT * 2] = {0};
    GameSceneAssetHeader header;
    for (s32 i = 0; i < SCENE_ASSET_BLOCK_COUNT; ++i) {
        header.offsets[i] = (s32)sizeof(header) + i * 4;
        words[SCENE_ASSET_BLOCK_COUNT + i] = (u32)i + 1;
    }
    memcpy(words, &header, sizeof(header));
    SceneAssetBlock blocks[SCENE_ASSET_BLOCK_COUNT];
    CHECK(ReadSceneAssetBlocks(words, sizeof(words), blocks));
    for (s32 i = 0; i < SCENE_ASSET_BLOCK_COUNT; ++i) {
        CHECK(blocks[i].data == &words[SCENE_ASSET_BLOCK_COUNT + i]);
        CHECK(blocks[i].size == 4 && *(const u32 *)blocks[i].data == (u32)i + 1);
    }
    SceneAssetBlock before[SCENE_ASSET_BLOCK_COUNT];
    memcpy(before, blocks, sizeof(blocks));
    for (size_t size = 0; size < sizeof(words) - 3; ++size) {
        CHECK(!ReadSceneAssetBlocks(words, size, blocks));
        CHECK(memcmp(blocks, before, sizeof(blocks)) == 0);
    }
    CHECK(!ReadSceneAssetBlocks(NULL, sizeof(words), blocks));
    CHECK(!ReadSceneAssetBlocks(words, sizeof(words), NULL));
    for (s32 i = 0; i < SCENE_ASSET_BLOCK_COUNT; ++i) {
        const u32 original = words[i];
        const s32 invalid[] = {-1, 0, (s32)sizeof(header) - 4,
                              (s32)sizeof(words), header.offsets[i] + 1};
        for (size_t j = 0; j < sizeof(invalid) / sizeof(invalid[0]); ++j) {
            words[i] = (u32)invalid[j];
            CHECK(!ReadSceneAssetBlocks(words, sizeof(words), blocks));
            CHECK(memcmp(blocks, before, sizeof(blocks)) == 0);
        }
        words[i] = original;
    }
    u32 misaligned[SCENE_ASSET_BLOCK_COUNT * 2 + 1];
    memcpy((u8 *)misaligned + 1, words, sizeof(words));
    CHECK(!ReadSceneAssetBlocks((u8 *)misaligned + 1, sizeof(words), blocks));
    CHECK(memcmp(blocks, before, sizeof(blocks)) == 0);
    u32 other[SCENE_ASSET_BLOCK_COUNT * 2];
    memcpy(other, words, sizeof(words));
    CHECK(ReadSceneAssetBlocks(other, sizeof(other), blocks));
    CHECK(blocks[0].data == &other[SCENE_ASSET_BLOCK_COUNT]);
    CHECK(before[0].data == &words[SCENE_ASSET_BLOCK_COUNT]);
    SceneAsset *owned = CopySceneAsset(words, sizeof(words));
    SceneAsset *second = CopySceneAsset(words, sizeof(words));
    CHECK(owned && second && owned->blocks[0].data != second->blocks[0].data);
    CHECK(CopySceneAsset(NULL, sizeof(words)) == NULL);
    CHECK(CopySceneAsset(words, sizeof(header)) == NULL);
    memset(words, 0, sizeof(words));
    for (s32 i = 0; i < SCENE_ASSET_BLOCK_COUNT; ++i) {
        CHECK(owned->blocks[i].size == 4);
        CHECK(*(const u32 *)owned->blocks[i].data == (u32)i + 1);
    }
    FreeSceneAsset(owned);
    CHECK(*(const u32 *)second->blocks[10].data == 11);
    FreeSceneAsset(second);
    FreeSceneAsset(NULL);
    return 0;
}
