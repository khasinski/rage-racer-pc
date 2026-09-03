#include "game/memcard.h"
#include "game/save_internal.h"

#include <stdio.h>
#include <string.h>

static Rect s_rects[2];
static void *s_destinations[2];
static s32 s_storeCalls;
static s32 s_syncCalls;

void StoreImage(Rect *rect, void *data) {
    s_rects[s_storeCalls] = *rect;
    s_destinations[s_storeCalls] = data;
    s_storeCalls++;
}

void DrawSync(long mode) {
    if (mode == 0) s_syncCalls++;
}

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", \
                __FILE__, __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void) {
    GameSaveIconBlock block;
    char longTitle[160];

    memset(&block, 0xCC, sizeof(block));
    memset(longTitle, 'A', sizeof(longTitle) - 1);
    longTitle[sizeof(longTitle) - 1] = '\0';

    BuildSaveIconBlock(&block, longTitle);

    CHECK(block.magic[0] == 'S' && block.magic[1] == 'C');
    CHECK(block.format == 0x11 && block.frameCount == 1);
    CHECK(block.title[sizeof(block.title) - 1] == '\0');
    CHECK(block.clut[0] == 0xCCCC);
    CHECK(s_storeCalls == 2 && s_syncCalls == 2);
    CHECK(s_destinations[0] == block.clut);
    CHECK(s_rects[0].x == 0x60 && s_rects[0].y == 0x1FB);
    CHECK(s_rects[0].w == 0x10 && s_rects[0].h == 1);
    CHECK(s_destinations[1] == block.pixels);
    CHECK(s_rects[1].x == 0x3C0 && s_rects[1].y == 0x1F0);
    CHECK(s_rects[1].w == 4 && s_rects[1].h == 0x10);

    puts("save icon headers bound titles and capture both GPU regions");
    return 0;
}
