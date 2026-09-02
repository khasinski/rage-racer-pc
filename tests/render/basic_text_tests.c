#include "common.h"
#include "game/menu.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/render_state.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
GameFrameContext g_FrameContexts[2];
GameFrameContext *g_DrawBuffer;
u8 g_Font8x8Cells[192];
u8 g_DrawModeEnv[8];
u8 g_SpriteFontCells[192];
u8 g_SpriteFontWidth[96];

static union {
    max_align_t alignment;
    u8 bytes[2048];
} s_packets;
static s32 s_failures;

u8 *AddTilePrim(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y, s32 w,
                s32 h,
                s32 r, s32 g, s32 b) {
    (void)ot; (void)x; (void)y; (void)w; (void)h;
    (void)r; (void)g; (void)b;
    return prim;
}

static void Check(s32 actual, s32 expected, const char *label) {
    if (actual != expected) {
        printf("FAIL %s: got %d, expected %d\n", label, actual, expected);
        s_failures++;
    }
}

static void Reset(void) {
    memset(&s_packets, 0, sizeof(s_packets));
    memset(&g_FrameContexts[0].layout.orderingTables[0][0], 0,
           sizeof(GameOrderingTableEntry));
    g_RenderState.packetCursor = s_packets.bytes;
    g_DrawBuffer = &g_FrameContexts[0];
}

static void TestText8x8(void) {
    SPRT_8 *sprite;
    s32 glyph = 'A' - 0x20;

    g_Font8x8Cells[glyph * 2] = 3;
    g_Font8x8Cells[glyph * 2 + 1] = 5;

    Reset();
    DrawText8x8(10, 20, " A", 0x123);
    sprite = (SPRT_8 *)s_packets.bytes;
    Check(sprite->x0, 18, "8x8 space advance");
    Check(sprite->y0, 20, "8x8 y");
    Check(sprite->u0, 24, "8x8 u");
    Check(sprite->v0, 40, "8x8 v");
    Check(sprite->clut, 0x123, "8x8 clut");
    Check(sprite->code & 3, 1, "8x8 raw opaque flags");
    Check((u8 *)g_RenderState.packetCursor - s_packets.bytes,
          (s32)(sizeof(SPRT_8) + sizeof(DrawPacket)), "8x8 cursor");

    Reset();
    GameDrawText8x8Shaded(1, 2, "A", 7, 0x55);
    sprite = (SPRT_8 *)s_packets.bytes;
    Check(sprite->r0, 0x55, "shaded text red");
    Check(sprite->g0, 0x55, "shaded text green");
    Check(sprite->b0, 0x55, "shaded text blue");
    Check(sprite->code & 3, 2, "shaded text flags");

    Reset();
    DrawText8x8Trans(1, 2, "A", 7);
    sprite = (SPRT_8 *)s_packets.bytes;
    Check(sprite->code & 3, 3, "raw transparent text flags");

    Reset();
    DrawText8x8(0, 0, "", 0);
    Check((u8 *)g_RenderState.packetCursor - s_packets.bytes,
          (s32)sizeof(DrawPacket), "empty text draw-mode packet");
}

static void TestSpriteString(void) {
    SPRT *sprite;
    s32 glyph = 'A' - 0x20;

    g_SpriteFontWidth[0] = 4;
    g_SpriteFontWidth[glyph] = 9;
    g_SpriteFontCells[glyph * 2] = 12;
    g_SpriteFontCells[glyph * 2 + 1] = 34;

    Reset();
    DrawSpriteString(10, 30, " A", 0x234);
    sprite = (SPRT *)s_packets.bytes;
    Check(sprite->x0, 14, "sprite string space advance");
    Check(sprite->y0, 30, "sprite string y");
    Check(sprite->u0, 12, "sprite string u");
    Check(sprite->v0, 34, "sprite string v");
    Check(sprite->w, 9, "sprite string width");
    Check(sprite->h, 0x18, "sprite string height");
    Check(sprite->clut, 0x234, "sprite string clut");
    Check(sprite->code & 3, 1, "sprite string flags");
    Check((u8 *)g_RenderState.packetCursor - s_packets.bytes,
          (s32)(sizeof(SPRT) + sizeof(DrawPacket)), "sprite string cursor");
}

int main(void) {
    TestText8x8();
    TestSpriteString();
    if (s_failures != 0) return 1;
    puts("basic text emitters queue glyph and draw-mode packets");
    return 0;
}
