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
    InitBlock(&entry.clut, sizeof(entry.clut), 10, 20, 16, 1, 0xA1);
    InitBlock(&entry.pixels, sizeof(entry.pixels), 30, 40, 64, 32, 0xB2);
    s_loadCount = 0;
    s_syncCount = 0;
    UploadImageEntry(&entry.header);
    Check(s_loadCount == 2 && s_syncCount == 2,
          "CLUT entry uploads two synchronized blocks");
    Check(s_loadRects[0].x == 10 && s_loadRects[0].y == 20 &&
              s_loadRects[0].w == 16 && s_loadRects[0].h == 1 &&
              s_loadData[0] == entry.clut.pixels,
          "CLUT upload rectangle and pixels");
    Check(s_loadRects[1].x == 30 && s_loadRects[1].y == 40 &&
              s_loadRects[1].w == 64 && s_loadRects[1].h == 32 &&
              s_loadData[1] == entry.pixels.pixels,
          "image upload rectangle and pixels");

    memset(&noClut, 0, sizeof(noClut));
    InitBlock(&noClut.pixels, sizeof(noClut.pixels), 50, 60, 0, 8, 0xC3);
    s_loadCount = 0;
    UploadImageEntry(&noClut.header);
    Check(s_loadCount == 0, "empty image dimensions skip upload");
    noClut.pixels.w = 8;
    UploadImageEntry(&noClut.header);
    Check(s_loadCount == 1 && s_loadRects[0].x == 50,
          "entry without CLUT uploads its image directly");
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
    GameImageBlock *block;
    u8 *cursor;
    const s32 payloadSize = sizeof(GameImageEntryHeader) +
                            sizeof(GameImageBlock);

    memset(&chain, 0, sizeof(chain));
    cursor = chain.bytes + sizeof(*words);
    ((GameImageAssetHeaderWord *)(void *)cursor)->size = payloadSize;
    cursor += sizeof(*words);
    first = (GameImageEntryHeader *)(void *)cursor;
    block = (GameImageBlock *)(void *)(first + 1);
    InitBlock(block, sizeof(*block), 1, 2, 3, 4, 0x11);
    cursor += payloadSize;
    ((GameImageAssetHeaderWord *)(void *)cursor)->size = payloadSize;
    cursor += sizeof(*words);
    second = (GameImageEntryHeader *)(void *)cursor;
    block = (GameImageBlock *)(void *)(second + 1);
    InitBlock(block, sizeof(*block), 5, 6, 7, 8, 0x22);
    cursor += payloadSize;
    ((GameImageAssetHeaderWord *)(void *)cursor)->size = 0;

    s_loadCount = 0;
    UploadImageAsset(words);
    Check(s_loadCount == 2 && s_loadRects[0].x == 1 &&
              s_loadRects[1].x == 5,
          "image asset walks every positive-size entry");

    memcpy(g_LoadBuffer, chain.bytes, sizeof(chain.bytes));
    s_loadCount = 0;
    UploadLoadBufferImage();
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
