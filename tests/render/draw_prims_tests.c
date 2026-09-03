#include "common.h"
#include "game/render.h"
#include "game/render_state.h"
#include "psyq/gpu.h"

#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;

static union {
    max_align_t alignment;
    u8 bytes[2048];
} s_packets;
static GameOrderingTableEntry s_ot;
static s32 s_queuePages[16];
static u8 *s_queuePackets[16];
static s32 s_queueCount;
static int s_failures;

#define CHECK_EQ(actual, expected, label) do { \
    s32 actualValue = (s32)(actual); \
    s32 expectedValue = (s32)(expected); \
    if (actualValue != expectedValue) { \
        printf("FAIL %s: got %d, expected %d\n", \
               label, actualValue, expectedValue); \
        s_failures++; \
    } \
} while (0)

u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *packet, s32 tpage) {
    (void)ot;
    s_queuePackets[s_queueCount] = packet;
    s_queuePages[s_queueCount] = tpage;
    s_queueCount++;
    return packet + sizeof(DrawPacket);
}

static void ResetPackets(void) {
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    memset(&s_packets, 0, sizeof(s_packets));
    memset(&s_ot, 0, sizeof(s_ot));
    memset(s_queuePages, 0, sizeof(s_queuePages));
    memset(s_queuePackets, 0, sizeof(s_queuePackets));
    s_queueCount = 0;
    g_RenderState.packetCursor = s_packets.bytes;
}

static void CheckFlatQuad(void) {
    POLY_F4 *quad;

    ResetPackets();
    DrawFlatQuad(&s_ot, -2, -3, 10, 11, 20, 21, 30, 31,
                 40, 50, 60, 1, 0x1234);
    quad = (POLY_F4 *)s_packets.bytes;
    CHECK_EQ(quad->x0, -2, "quad x0");
    CHECK_EQ(quad->y3, 31, "quad y3");
    CHECK_EQ(quad->r0, 40, "quad red");
    CHECK_EQ(quad->g0, 50, "quad green");
    CHECK_EQ(quad->b0, 60, "quad blue");
    CHECK_EQ((quad->code & 2) != 0, 1, "quad semitransparency");
    CHECK_EQ(s_queueCount, 1, "quad draw mode count");
    CHECK_EQ(s_queuePages[0], 0x1234, "quad draw mode");
    CHECK_EQ(s_queuePackets[0] == (u8 *)(quad + 1), 1,
             "quad draw mode position");
    CHECK_EQ(g_RenderState.packetCursor ==
                 s_queuePackets[0] + sizeof(DrawPacket),
             1, "quad cursor");

    ResetPackets();
    DrawFlatQuad(&s_ot, 0, 0, 1, 1, 2, 2, 3, 3,
                 4, 5, 6, 0, 0x80);
    CHECK_EQ(s_queueCount, 0, "quad embedded draw mode");
    CHECK_EQ(g_RenderState.packetCursor ==
                 s_packets.bytes + sizeof(POLY_F4),
             1, "quad cursor without draw mode");
}

static void CheckSpriteAndPolygons(void) {
    SPRT *sprite;
    POLY_F3 *triangle;
    POLY_FT4 *textured;
    LINE_F3 *line;

    ResetPackets();
    DrawSprite(&s_ot, -1, 2, 30, 40, 5, 6, 7, 8, 9,
               21, 1, 1, 0x34);
    sprite = (SPRT *)s_packets.bytes;
    CHECK_EQ(sprite->x0, -1, "sprite x");
    CHECK_EQ(sprite->w, 30, "sprite width");
    CHECK_EQ(sprite->u0, 5, "sprite u");
    CHECK_EQ(sprite->clut, ((0x1E0 + 1) << 6) + 1, "sprite clut");
    CHECK_EQ((sprite->code & 3), 3, "sprite texture flags");
    CHECK_EQ(s_queuePages[0], 0x34, "sprite draw mode");

    ResetPackets();
    DrawFlatTriangle(&s_ot, 1, 2, 3, 4, 5, 6,
                     7, 8, 9, 1, 0x80);
    triangle = (POLY_F3 *)s_packets.bytes;
    CHECK_EQ(triangle->x2, 5, "triangle x2");
    CHECK_EQ(triangle->b0, 9, "triangle blue");
    CHECK_EQ((triangle->code & 2) != 0, 1, "triangle semitransparency");
    CHECK_EQ(s_queueCount, 0, "triangle embedded draw mode");

    ResetPackets();
    GameDrawTexturedQuad(&s_ot, 1, 2, 3, 4, 5, 6, 7, 8,
                         9, 10, 11, 12, 13, 14, 15, 16,
                         17, 18, 19, 39, 1, 0, 0x55);
    textured = (POLY_FT4 *)s_packets.bytes;
    CHECK_EQ(textured->x3, 7, "textured quad x3");
    CHECK_EQ(textured->u3, 15, "textured quad u3");
    CHECK_EQ(textured->b0, 19, "textured quad blue");
    CHECK_EQ(textured->clut, ((0x1E0 + 1) << 6) + 19,
             "textured quad clut");
    CHECK_EQ(textured->tpage, 0x55, "textured quad page");
    CHECK_EQ(g_RenderState.packetCursor ==
                 s_packets.bytes + sizeof(POLY_FT4),
             1, "textured quad cursor");

    ResetPackets();
    DrawPolyLine3(&s_ot, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0x22);
    line = (LINE_F3 *)s_packets.bytes;
    CHECK_EQ(line->x2, 5, "polyline x2");
    CHECK_EQ(line->b0, 9, "polyline blue");
    CHECK_EQ((line->code & 2) != 0, 1, "polyline semitransparency");
    CHECK_EQ(s_queuePages[0], 0x22, "polyline draw mode");
}

static void CheckSolidAndLines(void) {
    TILE *tile;
    LINE_G2 *gradient;

    ResetPackets();
    DrawSolidRect(&s_ot, -7, 8, 9, 10, 0x123, 0x145, 0x167, 0x22);
    tile = (TILE *)s_packets.bytes;
    CHECK_EQ(tile->x0, -7, "tile x");
    CHECK_EQ(tile->h, 10, "tile height");
    CHECK_EQ(tile->r0, 0x23, "tile red truncation");
    CHECK_EQ(tile->g0, 0x45, "tile green truncation");
    CHECK_EQ(tile->b0, 0x67, "tile blue truncation");
    CHECK_EQ((tile->code & 2) != 0, 1, "tile semitransparency");
    CHECK_EQ(s_queuePages[0], 0x22, "tile draw mode");

    ResetPackets();
    DrawGradientLine(&s_ot, -1, -2, 30, 40,
                     1, 2, 3, 4, 5, 6, 0xFF);
    gradient = (LINE_G2 *)s_packets.bytes;
    CHECK_EQ(gradient->x0, -1, "gradient x0");
    CHECK_EQ(gradient->y1, 40, "gradient y1");
    CHECK_EQ(gradient->r0, 1, "gradient first red");
    CHECK_EQ(gradient->b1, 6, "gradient second blue");
    CHECK_EQ((gradient->code & 2) != 0, 0, "opaque gradient");
    CHECK_EQ(s_queueCount, 0, "opaque gradient draw mode");
}

static void CheckRectOutline(void) {
    LINE_F2 *lines;

    ResetPackets();
    DrawRectOutline(&s_ot, 10, 20, 30, 40, 1, 2, 3, 0xFF);
    lines = (LINE_F2 *)s_packets.bytes;
    CHECK_EQ(lines[0].x0, 10, "outline top left");
    CHECK_EQ(lines[0].x1, 39, "outline top right");
    CHECK_EQ(lines[1].y0, 21, "outline second row");
    CHECK_EQ(lines[2].x1, 10, "outline left side");
    CHECK_EQ(lines[2].y0, 22, "outline side top");
    CHECK_EQ(lines[2].y1, 57, "outline side bottom");
    CHECK_EQ(lines[3].x0, 39, "outline right side");
    CHECK_EQ(lines[4].y0, 59, "outline bottom");
    CHECK_EQ(lines[5].y0, 58, "outline penultimate row");
    CHECK_EQ(s_queueCount, 0, "opaque outline draw modes");
    CHECK_EQ(g_RenderState.packetCursor ==
                 s_packets.bytes + sizeof(LINE_F2) * 6,
             1, "outline cursor");

    ResetPackets();
    DrawRectOutline(&s_ot, INT32_MAX, INT32_MIN, INT32_MAX, INT32_MIN,
                    1, 2, 3, 0xFF);
    lines = (LINE_F2 *)s_packets.bytes;
    CHECK_EQ(lines[0].x0, -1, "extreme outline wrapped left");
    CHECK_EQ(lines[0].x1, -3, "extreme outline wrapped right");
    CHECK_EQ(lines[0].y0, 0, "extreme outline wrapped top");
    CHECK_EQ(lines[4].y0, -1, "extreme outline wrapped bottom");
}

static void CheckClipRect(void) {
    DrawPacket expected;
    DrawPacket *actual;
    Rect rect = {0, 0, 320, 480};

    ResetPackets();
    memset(&expected, 0, sizeof(expected));
    SetDrawArea(&expected, &rect);
    SetDrawClipRect(&s_ot, -5, -7, 400, 500);
    actual = (DrawPacket *)s_packets.bytes;
    CHECK_EQ(actual->code[0], expected.code[0], "clip top-left code");
    CHECK_EQ(actual->code[1], expected.code[1], "clip bottom-right code");
    CHECK_EQ(g_RenderState.packetCursor ==
                 s_packets.bytes + sizeof(DrawPacket),
             1, "clip cursor");

    ResetPackets();
    SetDrawClipRect(&s_ot, -20, 10, 10, 20);
    CHECK_EQ(g_RenderState.packetCursor == s_packets.bytes, 1,
             "clip rejects left rectangle");
    SetDrawClipRect(&s_ot, 320, 10, 1, 20);
    CHECK_EQ(g_RenderState.packetCursor == s_packets.bytes, 1,
             "clip rejects right rectangle");
    SetDrawClipRect(&s_ot, 10, -20, 20, 10);
    CHECK_EQ(g_RenderState.packetCursor == s_packets.bytes, 1,
             "clip rejects top rectangle");
    SetDrawClipRect(&s_ot, 10, 480, 20, 1);
    CHECK_EQ(g_RenderState.packetCursor == s_packets.bytes, 1,
             "clip rejects bottom rectangle");

    SetDrawClipRect(&s_ot, 10, 10, 0, 20);
    SetDrawClipRect(&s_ot, 10, 10, -1, 20);
    SetDrawClipRect(&s_ot, 10, 10, 20, 0);
    SetDrawClipRect(&s_ot, 10, 10, 20, -1);
    CHECK_EQ(g_RenderState.packetCursor == s_packets.bytes, 1,
             "clip rejects non-positive dimensions");

    SetDrawClipRect(&s_ot, INT32_MAX, 10, 20, 20);
    SetDrawClipRect(&s_ot, 10, INT32_MAX, 20, 20);
    CHECK_EQ(g_RenderState.packetCursor == s_packets.bytes, 1,
             "clip rejects overflowing off-screen coordinates");

    SetDrawClipRect(&s_ot, INT32_MIN, 10, INT32_MAX, 20);
    SetDrawClipRect(&s_ot, 10, INT32_MIN, 20, INT32_MAX);
    CHECK_EQ(g_RenderState.packetCursor == s_packets.bytes, 1,
             "clip rejects extreme rectangles before the screen");
}

int main(void) {
    CheckFlatQuad();
    CheckSpriteAndPolygons();
    CheckSolidAndLines();
    CheckRectOutline();
    CheckClipRect();
    if (s_failures != 0) return 1;
    puts("draw primitive packets passed");
    return 0;
}
