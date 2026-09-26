#include "game/image_asset.h"
#include "game/track_images.h"
#include <stdlib.h>
#include <stdio.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
#include <string.h>

int main(void) {
    /* Header, entry size, entry header, 2x1 pixel block, terminator. */
    u32 words[] = {0, 24, 0, 0, 16, 0, 0x00010002, 0x001f001f, 0};
    const GameImageAssetHeaderWord *asset = (const void *)words;
    CHECK(IsValidImageAsset(asset, sizeof(words)));
    for (size_t size = 0; size < sizeof(words); ++size)
        CHECK(!IsValidImageAsset(asset, size));
    CHECK(!IsValidImageAsset(NULL, sizeof(words)));
    words[1] = 28; /* Declared entry consumes the required terminator. */
    CHECK(!IsValidImageAsset(asset, sizeof(words)));
    words[1] = 24;
    words[5] = 1023; /* Width two crosses the image boundary. */
    CHECK(!IsValidImageAsset(asset, sizeof(words)));
    words[5] = 0;
    words[6] = 0x00020002; /* Four pixels but only two words stored. */
    CHECK(!IsValidImageAsset(asset, sizeof(words)));
    words[6] = 0x00010002;
    words[3] = GAME_IMAGE_ENTRY_HAS_CLUT; /* Missing second block. */
    CHECK(!IsValidImageAsset(asset, sizeof(words)));
    words[3] = 0;
    u32 copy[sizeof(words) / sizeof(words[0])];
    memcpy(copy, words, sizeof(words));
    CHECK(IsValidImageAsset(asset, sizeof(words)));
    CHECK(memcmp(copy, words, sizeof(words)) == 0);
    /* Same valid payload at every possible word alignment. */
    u8 bytes[sizeof(words) + 4];
    for (size_t offset = 0; offset < 4; ++offset) {
        memcpy(bytes + offset, words, sizeof(words));
        CHECK(IsValidImageAsset((const void *)(bytes + offset), sizeof(words)));
        for (size_t size = 0; size < sizeof(words); ++size)
            CHECK(!IsValidImageAsset((const void *)(bytes + offset), size));
    }
    /* A palette block followed by a separate pixel block. */
    u32 indexed[] = {0, 40, 0, GAME_IMAGE_ENTRY_HAS_CLUT,
                     16, 0, 0x00010002, 0x001f001f,
                     16, 0, 0x00010002, 0x11111111, 0};
    CHECK(IsValidImageAsset((const void *)indexed, sizeof(indexed)));
    for (size_t size = 0; size < sizeof(indexed); ++size)
        CHECK(!IsValidImageAsset((const void *)indexed, size));
    indexed[4] = UINT32_MAX;
    CHECK(!IsValidImageAsset((const void *)indexed, sizeof(indexed)));
    indexed[4] = 17; /* Misaligned block chain. */
    CHECK(!IsValidImageAsset((const void *)indexed, sizeof(indexed)));
    indexed[4] = 12; /* Palette header declares no room for its pixels. */
    CHECK(!IsValidImageAsset((const void *)indexed, sizeof(indexed)));
    CHECK(!IsValidImageEntry(NULL, sizeof(indexed)));
    u16 output[] = {0xaaaa, 0xbbbb, 0xcccc, 0xdddd};
    ImagePixels image = {output, 4, 0, 0, 2, 2};
    CHECK(CopyImageAssetPixels(words, sizeof(words), &image));
    CHECK(output[0] == 0x001f && output[1] == 0x001f);
    CHECK(output[2] == 0xcccc && output[3] == 0xdddd);
    /* A cropped destination receives only the overlapping column. */
    image.x = 1;
    image.width = 1;
    output[0] = output[1] = 0xbeef;
    CHECK(CopyImageAssetPixels(words, sizeof(words), &image));
    CHECK(output[0] == 0x001f && output[1] == 0xbeef);
    u16 before[4];
    CHECK(CopyImageEntryPixels(words + 2, 24, &image));
    memcpy(before, output, sizeof(output));
    for (size_t size = 0; size < 24; ++size) {
        CHECK(!CopyImageEntryPixels(words + 2, size, &image));
        CHECK(memcmp(before, output, sizeof(output)) == 0);
    }
    CHECK(!CopyImageEntryPixels(NULL, 24, &image));

    CHECK(!CopyImageAssetPixels(words, sizeof(words) - 1, &image));
    image.count = 1;
    CHECK(!CopyImageAssetPixels(words, sizeof(words), &image));
    image.count = 4;
    image.x = UINT32_MAX;
    CHECK(!CopyImageAssetPixels(words, sizeof(words), &image));
    CHECK(memcmp(before, output, sizeof(output)) == 0);
    image = (ImagePixels){output, 4, 0, 0, 2, 2};
    indexed[4] = 16;
    CHECK(CopyImageAssetPixels(indexed, sizeof(indexed), &image));
    CHECK(output[0] == 0x1111 && output[1] == 0x1111); /* Later block wins. */
    for (size_t offset = 0; offset < 4; ++offset) {
        memcpy(bytes + offset, words, sizeof(words));
        CHECK(CopyImageAssetPixels(bytes + offset, sizeof(words), &image));
    }
    const size_t packSize = TRACK_TEXTURE_SHADOW_SIZE + 8;
    u8 *pack = calloc(1, packSize + 1);
    CHECK(pack != NULL);
    const TrackTextureAssetHeader packHeader = {{20, 28, 36, 56, TRACK_TEXTURE_SHADOW_SIZE}};
    memcpy(pack, &packHeader, sizeof(packHeader));
    TrackTextureAssetView view;
    CHECK(ReadTrackImages(pack, packSize, &view));
    CHECK(view.blocks[TRACK_TEXTURE_CAR_IMAGE] == pack + 36);
    CHECK(view.sizes[TRACK_TEXTURE_CAR_IMAGE] == 20);
    TrackTextureAssetView saved = view;
    CHECK(!ReadTrackImages(NULL, packSize, &view));
    CHECK(!ReadTrackImages(pack, packSize, NULL));
    CHECK(!ReadTrackImages(pack, TRACK_TEXTURE_SHADOW_SIZE - 1, &view));
    for (s32 i = 0; i < TRACK_TEXTURE_BLOCK_COUNT; ++i) {
        const s32 invalid = -1;
        memcpy(pack + i * sizeof(s32), &invalid, sizeof(invalid));
        CHECK(!ReadTrackImages(pack, packSize, &view));
        CHECK(memcmp(&view, &saved, sizeof(view)) == 0);
        memcpy(pack, &packHeader, sizeof(packHeader));
    }
    const s32 brokenEntry = 4;
    memcpy(pack + TRACK_TEXTURE_SHADOW_SIZE + 4, &brokenEntry, sizeof(brokenEntry));
    CHECK(!ReadTrackImages(pack, packSize, &view));
    CHECK(memcmp(&view, &saved, sizeof(view)) == 0);
    memset(pack + TRACK_TEXTURE_SHADOW_SIZE + 4, 0, sizeof(s32));
    TrackImages *owned = CopyTrackImages(pack, packSize);
    TrackImages *independent = CopyTrackImages(pack, packSize);
    CHECK(owned && independent);
    CHECK(owned->view.blocks[0] != independent->view.blocks[0]);
    CHECK(CopyTrackImages(NULL, packSize) == NULL);
    CHECK(CopyTrackImages(pack, TRACK_TEXTURE_SHADOW_SIZE) == NULL);
    memmove(pack + 1, pack, packSize);
    CHECK(ReadTrackImages(pack + 1, packSize, &view));
    CHECK(view.blocks[0] == pack + 21);
    memset(pack, 0xff, packSize + 1);
    free(pack);
    CHECK(IsValidImageAsset(owned->view.blocks[0], owned->view.sizes[0]));
    CHECK(IsValidImageEntry(owned->view.blocks[2], owned->view.sizes[2]));
    FreeTrackImages(owned);
    CHECK(IsValidImageAsset(independent->view.blocks[4], independent->view.sizes[4]));
    FreeTrackImages(independent);
    FreeTrackImages(NULL);

    return 0;
}
