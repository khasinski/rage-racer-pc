#include "common.h"
#include "game/asset.h"
#include "game/race.h"

#include <stdio.h>
#include <string.h>

u16 g_TeamLogoClut[16];
Rect g_TeamLogoClutLoadRect;
GpuRectPacked g_TeamLogoClutMoveRect;
Rect g_TrackTextureRect;
s16 g_GrandPrixSeries;
s32 g_LoadBuffer[64];
size_t g_LoadBufferImageSize;
TrackTextureShadowRow *g_TrackTextureShadow;
u8 *g_AssetLoadCursor;

static Rect s_loadRects[8];
static void *s_loadData[8];
static s32 s_loadCount;
static Rect *s_storeRect;
static void *s_storeData;
static s32 s_storeClutAtCall;
static GpuRectPacked *s_moveRect;
static u_long s_moveX;
static u_long s_moveY;
static s32 s_moveCount;
static s32 s_syncCount;
static s32 s_failures;
static u16 s_vram[512][1024];
static s32 s_copyPixels;
static s32 s_textureResets;
static s32 s_textureRevisions;

void ResetTrackTextureSwap(void) { s_textureResets++; }
void TrackAssetIdentityInvalidate(void) { s_textureRevisions++; }

void LoadImage(Rect *rect, void *data) {
    s32 row;
    if (s_copyPixels) {
        for (row = 0; row < rect->h; row++)
            memcpy(&s_vram[rect->y + row][rect->x],
                   (u16 *)data + row * rect->w, rect->w * sizeof(u16));
    }
    s_loadRects[s_loadCount] = *rect;
    s_loadData[s_loadCount] = data;
    s_loadCount++;
}
void StoreImage(Rect *rect, void *data) {
    s32 row;
    if (s_copyPixels) {
        for (row = 0; row < rect->h; row++)
            memcpy((u16 *)data + row * rect->w,
                   &s_vram[rect->y + row][rect->x], rect->w * sizeof(u16));
    }
    s_storeRect = rect;
    s_storeData = data;
    s_storeClutAtCall = g_TeamLogoClut[0];
}
long MoveImage(GpuRectPacked *rect, u_long x, u_long y) {
    s_moveRect = rect;
    s_moveX = x;
    s_moveY = y;
    s_moveCount++;
    return 0;
}
void DrawSync(long mode) {
    (void)mode;
    s_syncCount++;
}

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

static void InitBlock(GameImageBlock *block, u32 size, s16 x, s16 y,
                      s16 width, s16 height, u8 pixel) {
    memset(block, 0, sizeof(*block));
    block->size = size;
    block->x = (u16)x;
    block->y = (u16)y;
    block->w = (u16)width;
    block->h = (u16)height;
    block->pixels[0] = pixel;
}

static void TestImageEntries(void) {
    struct {
        GameImageEntryHeader header;
        GameImageBlock clut;
        GameImageBlock pixels;
    } entry;
    struct {
        GameImageEntryHeader header;
        GameImageBlock pixels;
    } noClut;
    u8 originalEntry[sizeof(entry)];

    memset(&entry, 0, sizeof(entry));
    entry.header.flags = GAME_IMAGE_ENTRY_HAS_CLUT;
    InitBlock(&entry.clut, sizeof(entry.clut), 10, 20, 2, 1, 0xA1);
    InitBlock(&entry.pixels, sizeof(entry.pixels), 30, 40, 2, 1, 0xB2);
    memcpy(originalEntry, &entry, sizeof(entry));
    s_loadCount = 0;
    s_syncCount = 0;
    Check(UploadImageEntry(&entry.header, sizeof(entry)) == 1,
          "complete CLUT entry is valid");
    Check(s_loadCount == 2 && s_syncCount == 2,
          "CLUT entry uploads two synchronized blocks");
    Check(memcmp(originalEntry, &entry, sizeof(entry)) == 0,
          "image entry remains unchanged during upload");
    Check(s_loadRects[0].x == 10 && s_loadRects[0].y == 20 &&
              s_loadRects[0].w == 2 && s_loadRects[0].h == 1 &&
              s_loadData[0] == entry.clut.pixels,
          "CLUT upload rectangle and pixels");
    Check(s_loadRects[1].x == 30 && s_loadRects[1].y == 40 &&
              s_loadRects[1].w == 2 && s_loadRects[1].h == 1 &&
              s_loadData[1] == entry.pixels.pixels,
          "image upload rectangle and pixels");

    memset(&noClut, 0, sizeof(noClut));
    InitBlock(&noClut.pixels, sizeof(noClut.pixels), 50, 60, 0, 1, 0xC3);
    s_loadCount = 0;
    Check(UploadImageEntry(&noClut.header, sizeof(noClut)) == 1,
          "empty image entry remains valid");
    Check(s_loadCount == 0, "empty image dimensions skip upload");
    noClut.pixels.w = 2;
    UploadImageEntry(&noClut.header, sizeof(noClut));
    Check(s_loadCount == 1 && s_loadRects[0].x == 50,
          "entry without CLUT uploads its image directly");

    s_loadCount = 0;
    entry.clut.size = sizeof(entry.clut) - 1;
    Check(UploadImageEntry(&entry.header, sizeof(entry)) == 0,
          "undersized CLUT block is invalid");
    Check(s_loadCount == 0, "undersized CLUT block rejects the entry");
    entry.clut.size = sizeof(entry.clut) + 1;
    Check(UploadImageEntry(&entry.header, sizeof(entry)) == 0,
          "unaligned CLUT block is invalid");
    Check(s_loadCount == 0, "unaligned CLUT block rejects the entry");

    UploadImageEntry(NULL, 0);
    Check(s_loadCount == 0, "null image entry is ignored");

    entry.clut.size = sizeof(entry.clut);
    Check(UploadImageEntry(&entry.header, sizeof(entry) - 1) == 0,
          "truncated pixel payload rejects the whole entry");

    entry.pixels.x = 1023;
    Check(UploadImageEntry(&entry.header, sizeof(entry)) == 0,
          "image crossing the right VRAM edge is invalid");
    entry.pixels.x = 30;
    entry.pixels.y = 511;
    entry.pixels.h = 2;
    Check(UploadImageEntry(&entry.header, sizeof(entry)) == 0,
          "image crossing the bottom VRAM edge is invalid");
}

static void TestImageAssetChain(void) {
    union {
        max_align_t alignment;
        u8 bytes[128];
    } chain;
    GameImageAssetHeaderWord *words =
        (GameImageAssetHeaderWord *)(void *)chain.bytes;
    GameImageEntryHeader *first;
    GameImageEntryHeader *second;
    GameImageAssetHeaderWord *secondLink;
    GameImageBlock *block;
    u8 *cursor;
    u8 originalChain[sizeof(chain.bytes)];
    size_t chainSize;
    const s32 payloadSize = sizeof(GameImageEntryHeader) +
                            sizeof(GameImageBlock);

    memset(&chain, 0, sizeof(chain));
    cursor = chain.bytes + sizeof(*words);
    ((GameImageAssetHeaderWord *)(void *)cursor)->size = payloadSize;
    cursor += sizeof(*words);
    first = (GameImageEntryHeader *)(void *)cursor;
    block = (GameImageBlock *)(void *)(first + 1);
    InitBlock(block, sizeof(*block), 1, 2, 2, 1, 0x11);
    cursor += payloadSize;
    secondLink = (GameImageAssetHeaderWord *)(void *)cursor;
    secondLink->size = payloadSize;
    cursor += sizeof(*words);
    second = (GameImageEntryHeader *)(void *)cursor;
    block = (GameImageBlock *)(void *)(second + 1);
    InitBlock(block, sizeof(*block), 5, 6, 2, 1, 0x22);
    cursor += payloadSize;
    ((GameImageAssetHeaderWord *)(void *)cursor)->size = 0;
    cursor += sizeof(*words);
    chainSize = (size_t)(cursor - chain.bytes);
    memcpy(originalChain, chain.bytes, sizeof(chain.bytes));

    s_loadCount = 0;
    Check(UploadImageAsset(words, chainSize) == 1,
          "terminated image chain is valid");
    Check(s_loadCount == 2 && s_loadRects[0].x == 1 &&
              s_loadRects[1].x == 5,
          "image asset walks every positive-size entry");
    Check(memcmp(originalChain, chain.bytes, sizeof(chain.bytes)) == 0,
          "image asset remains unchanged during upload");

    words[1].size = sizeof(GameImageEntryHeader) - 1;
    s_loadCount = 0;
    Check(UploadImageAsset(words, chainSize) == 0,
          "undersized entry invalidates the chain");
    Check(s_loadCount == 0, "undersized image entry stops the chain");
    words[1].size = payloadSize + 1;
    Check(UploadImageAsset(words, chainSize) == 0,
          "unaligned entry invalidates the chain");
    Check(s_loadCount == 0, "unaligned image entry stops the chain");
    words[1].size = payloadSize;

    secondLink->size = sizeof(GameImageEntryHeader) - 1;
    s_loadCount = 0;
    Check(UploadImageAsset(words, chainSize) == 0 && s_loadCount == 0,
          "invalid later entry causes no partial image upload");
    secondLink->size = payloadSize;

    UploadImageAsset(NULL, 0);
    Check(s_loadCount == 0, "null image asset is ignored");

    Check(UploadImageAsset(words, chainSize - sizeof(*words)) == 0,
          "unterminated image chain is rejected at its boundary");

    memcpy(g_LoadBuffer, chain.bytes, sizeof(chain.bytes));
    g_LoadBufferImageSize = chainSize;
    s_loadCount = 0;
    Check(UploadLoadBufferImage() == 1,
          "load buffer wrapper reports a valid image");
    Check(s_loadCount == 2, "load buffer wrapper uploads the same chain");
}

static void TestTeamLogoStorage(void) {
    u8 destination[32];

    g_TeamLogoClut[0] = 0;
    g_GrandPrixSeries = 0;
    s_loadCount = 0;
    s_moveCount = 0;
    s_syncCount = 0;
    StoreTeamLogoImage(destination);
    Check(s_loadCount == 1 && s_loadData[0] == g_TeamLogoClut,
          "team logo CLUT uploaded");
    Check(s_moveCount == 0, "Grand Prix keeps primary CLUT location");
    Check(s_storeRect == &g_TrackTextureRect && s_storeData == destination &&
              s_storeClutAtCall == CLUT_STP_BIT,
          "team logo texture stored with opaque black CLUT entry");
    Check(g_TeamLogoClut[0] == 0 && s_syncCount == 1,
          "team logo storage restores transparent CLUT entry");

    g_GrandPrixSeries = 1;
    StoreTeamLogoImage(destination);
    Check(s_moveCount == 1 && s_moveRect == &g_TeamLogoClutMoveRect &&
              s_moveX == 0x3F0 && s_moveY == 0xE2,
          "Extra Grand Prix moves the team logo CLUT");
}

/* Use the real pack installer and image decoder, with an in-memory GPU.
 * Both pages occupy the same rectangle; the first must survive the second
 * upload in the shadow. Recording only upload calls missed this regression. */
static size_t MakeTrackPage(u8 *asset, u16 seed) {
    const size_t pixels = 448u * 256u;
    const size_t entrySize = sizeof(GameImageEntryHeader) +
                            offsetof(GameImageBlock, pixels) + pixels * 2;
    GameImageEntryHeader *entry = (GameImageEntryHeader *)(void *)(asset + 8);
    GameImageBlock *block = (GameImageBlock *)(void *)(entry + 1);
    size_t i;
    *(u32 *)(void *)asset = 0;
    *(u32 *)(void *)(asset + 4) = (u32)entrySize;
    memset(entry, 0, sizeof(*entry));
    block->size = (u32)(entrySize - sizeof(*entry));
    block->x = 576; block->y = 256; block->w = 448; block->h = 256;
    for (i = 0; i < pixels; i++)
        ((u16 *)(void *)block->pixels)[i] = (u16)(seed + i);
    *(u32 *)(void *)(asset + 8 + entrySize) = 0;
    return 12 + entrySize;
}

static void TestDistinctTrackPages(void) {
    static u32 storage[120000];
    u8 *pack = (u8 *)storage;
    s32 *offsets = (s32 *)storage;
    size_t size, row, column;
    int correctShadow = 1, correctResident = 1;
    memset(storage, 0, sizeof(storage));
    offsets[0] = 64; offsets[1] = 96; offsets[2] = 128;
    offsets[3] = 160;
    /* The first two image lists and the car image are valid empty images. */
    offsets[4] = offsets[3] + (s32)MakeTrackPage(pack + offsets[3], 0x1234);
    size = (size_t)offsets[4] + MakeTrackPage(pack + offsets[4], 0x9876);
    g_GrandPrixSeries = 0;
    g_TeamLogoClutLoadRect = (Rect){0, 0, 16, 1};
    g_TrackTextureRect = (Rect){576, 256, 448, 256};
    s_copyPixels = 1;
    s_loadCount = 0;
    s_textureResets = s_textureRevisions = 0;
    Check(InstallTrackTextureAssetPack(pack, size), "two-page course installs");
    for (row = 0; row < 256; row++) {
        const u16 *shadow = (const u16 *)g_TrackTextureShadow[row];
        for (column = 0; column < 448; column++) {
            size_t index = row * 448 + column;
            if (shadow[column] != (u16)(0x1234 + index)) correctShadow = 0;
            if (s_vram[row + 256][column + 576] != (u16)(0x9876 + index))
                correctResident = 0;
        }
    }
    Check(correctShadow, "page 1 survives in shadow before page 0 overwrites VRAM");
    Check(correctResident, "page 0 occupies VRAM after installation");
    Check(g_AssetLoadCursor == pack + TRACK_TEXTURE_SHADOW_SIZE &&
              s_textureResets == 1 && s_textureRevisions == 1,
          "complete two-page pack publishes its cursor and texture generation");
    s_copyPixels = 0;
}

int main(void) {
    TestImageEntries();
    TestImageAssetChain();
    TestTeamLogoStorage();
    TestDistinctTrackPages();

    if (s_failures != 0) return 1;
    puts("image assets upload their CLUT, pixels, chain and team logo state");
    return 0;
}
