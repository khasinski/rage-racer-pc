#include "common.h"
#include "game/asset.h"
#include "game/menu_internal.h"
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

void LoadImage(Rect *rect, void *data) {
    s_loadRects[s_loadCount] = *rect;
    s_loadData[s_loadCount] = data;
    s_loadCount++;
}
void StoreImage(Rect *rect, void *data) {
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

    memset(&entry, 0, sizeof(entry));
    entry.header.flags = GAME_IMAGE_ENTRY_HAS_CLUT;
    InitBlock(&entry.clut, sizeof(entry.clut), 10, 20, 2, 1, 0xA1);
    InitBlock(&entry.pixels, sizeof(entry.pixels), 30, 40, 2, 1, 0xB2);
    s_loadCount = 0;
    s_syncCount = 0;
    Check(UploadImageEntry(&entry.header, sizeof(entry)) == 1,
          "complete CLUT entry is valid");
    Check(s_loadCount == 2 && s_syncCount == 2,
          "CLUT entry uploads two synchronized blocks");
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

    s_loadCount = 0;
    Check(UploadImageAsset(words, chainSize) == 1,
          "terminated image chain is valid");
    Check(s_loadCount == 2 && s_loadRects[0].x == 1 &&
              s_loadRects[1].x == 5,
          "image asset walks every positive-size entry");

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

int main(void) {
    TestImageEntries();
    TestImageAssetChain();
    TestTeamLogoStorage();

    if (s_failures != 0) return 1;
    puts("image assets upload their CLUT, pixels, chain and team logo state");
    return 0;
}
