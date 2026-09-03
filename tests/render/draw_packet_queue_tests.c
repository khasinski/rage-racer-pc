#include "game/prim.h"
#include "game/render.h"
#include "game/render_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

u8 g_DrawModeEnv[8];

static union {
    max_align_t alignment;
    u8 bytes[256];
} s_packets;
static GameOrderingTableEntry s_ot;
static s32 s_failures;

#define CHECK_EQ(actual, expected, label)                                      \
    do {                                                                       \
        s32 actualValue = (s32)(actual);                                       \
        s32 expectedValue = (s32)(expected);                                   \
        if (actualValue != expectedValue) {                                    \
            printf("FAIL %s: got %d, expected %d\n", label, actualValue,      \
                   expectedValue);                                             \
            s_failures++;                                                      \
        }                                                                      \
    } while (0)

static void ResetPackets(void) {
    memset(&s_packets, 0, sizeof(s_packets));
    memset(&s_ot, 0, sizeof(s_ot));
}

static void CheckShadedRect(void) {
    POLY_FT4 *packet;
    u8 *end;

    ResetPackets();
    end = GameQueueShadedTexturedRect(&s_ot, s_packets.bytes, 10, 20, 6, 8,
                                      30, 40, 0x123, 0x456, 70);
    packet = (POLY_FT4 *)s_packets.bytes;
    CHECK_EQ(end == s_packets.bytes + sizeof(*packet), 1, "shaded cursor");
    CHECK_EQ(packet->x0, 10, "shaded x0");
    CHECK_EQ(packet->y0, 20, "shaded y0");
    CHECK_EQ(packet->x3, 16, "shaded x3");
    CHECK_EQ(packet->y3, 28, "shaded y3");
    CHECK_EQ(packet->u0, 30, "shaded u0");
    CHECK_EQ(packet->v0, 40, "shaded v0");
    CHECK_EQ(packet->u3, 36, "shaded u3");
    CHECK_EQ(packet->v3, 48, "shaded v3");
    CHECK_EQ(packet->r0, 70, "shaded red");
    CHECK_EQ(packet->g0, 70, "shaded green");
    CHECK_EQ(packet->b0, 70, "shaded blue");
    CHECK_EQ(packet->clut, 0x123, "shaded clut");
    CHECK_EQ(packet->tpage, 0x456, "shaded tpage");

    ResetPackets();
    GameQueueShadedTexturedRect(&s_ot, s_packets.bytes, 1, 2, -6, -8, 30,
                                40, 0, 0, 0);
    packet = (POLY_FT4 *)s_packets.bytes;
    CHECK_EQ(packet->x3, 6, "shaded flipped x3");
    CHECK_EQ(packet->y3, 9, "shaded flipped y3");
    CHECK_EQ(packet->u0, 35, "shaded flipped u0");
    CHECK_EQ(packet->v0, 47, "shaded flipped v0");
    CHECK_EQ(packet->u3, 30, "shaded flipped u3");
    CHECK_EQ(packet->v3, 40, "shaded flipped v3");
}

static void CheckTexturedRect(void) {
    POLY_FT4 *packet;

    ResetPackets();
    GameQueueTexturedRect(&s_ot, s_packets.bytes, 3, 4, -10, -12, 50, 60,
                          -7, -9, 0x222, 0x333);
    packet = (POLY_FT4 *)s_packets.bytes;
    CHECK_EQ(packet->x3, 13, "textured x3");
    CHECK_EQ(packet->y3, 16, "textured y3");
    CHECK_EQ(packet->u0, 59, "textured u0");
    CHECK_EQ(packet->v0, 71, "textured v0");
    CHECK_EQ(packet->u3, 52, "textured u3");
    CHECK_EQ(packet->v3, 62, "textured v3");
    CHECK_EQ(packet->clut, 0x222, "textured clut");
    CHECK_EQ(packet->tpage, 0x333, "textured tpage");

    ResetPackets();
    GameQueueTexturedRect(&s_ot, s_packets.bytes, INT_MAX, INT_MAX, 10, 12,
                          0, 0, 10, 12, 0, 0);
    packet = (POLY_FT4 *)s_packets.bytes;
    CHECK_EQ(packet->x0, -1, "extreme textured x0 wraps");
    CHECK_EQ(packet->y0, -1, "extreme textured y0 wraps");
    CHECK_EQ(packet->x3, 9, "extreme textured x3 wraps");
    CHECK_EQ(packet->y3, 11, "extreme textured y3 wraps");
}

static void CheckDrawMode(void) {
    u8 *end;

    ResetPackets();
    end = QueueDrawModePrim(&s_ot, s_packets.bytes, 0x123);
    CHECK_EQ(end == s_packets.bytes + sizeof(DrawPacket), 1,
             "draw mode cursor");
}

static void CheckSpriteVariants(void) {
    SPRT *sprite;
    u8 *end;

    ResetPackets();
    sprite = (SPRT *)s_packets.bytes;
    end = GameQueueSprite(&s_ot, s_packets.bytes, 1, 2, 3, 4, 5, 6, 7);
    CHECK_EQ(end == (u8 *)(sprite + 1), 1, "raw sprite cursor");
    CHECK_EQ(sprite->code & 3, 1, "raw sprite flags");
    CHECK_EQ(sprite->x0, 1, "raw sprite x");
    CHECK_EQ(sprite->h, 4, "raw sprite height");
    CHECK_EQ(sprite->clut, 7, "raw sprite clut");

    ResetPackets();
    sprite = (SPRT *)s_packets.bytes;
    GameQueueSprite(&s_ot, s_packets.bytes, INT_MAX, INT_MIN, INT_MAX,
                    INT_MIN, 0, 0, 0);
    CHECK_EQ(sprite->x0, -1, "extreme sprite x wraps");
    CHECK_EQ(sprite->y0, 0, "extreme sprite y wraps");
    CHECK_EQ(sprite->w, -1, "extreme sprite width wraps");
    CHECK_EQ(sprite->h, 0, "extreme sprite height wraps");

    ResetPackets();
    sprite = (SPRT *)s_packets.bytes;
    end = GameQueueShadedSprite(&s_ot, s_packets.bytes, 8, 9, 10, 11,
                                12, 13, 14, 0x45);
    CHECK_EQ(end == (u8 *)(sprite + 1), 1, "shaded sprite cursor");
    CHECK_EQ(sprite->code & 3, 0, "shaded sprite flags");
    CHECK_EQ(sprite->r0, 0x45, "shaded sprite red");
    CHECK_EQ(sprite->g0, 0x45, "shaded sprite green");
    CHECK_EQ(sprite->b0, 0x45, "shaded sprite blue");

    ResetPackets();
    sprite = (SPRT *)s_packets.bytes;
    GameQueueShadedSpriteTrans(&s_ot, s_packets.bytes, 1, 2, 3, 4, 5, 6,
                               7, 0x67);
    CHECK_EQ(sprite->code & 3, 2, "transparent shaded sprite flags");
    CHECK_EQ(sprite->r0, 0x67, "transparent shaded sprite intensity");

    ResetPackets();
    sprite = (SPRT *)s_packets.bytes;
    GameQueueSpriteTrans(&s_ot, s_packets.bytes, 1, 2, 3, 4, 5, 6, 7);
    CHECK_EQ(sprite->code & 3, 3, "transparent raw sprite flags");
}

static void CheckLine(void) {
    LINE_F2 *line;
    u8 *end;

    ResetPackets();
    line = (LINE_F2 *)s_packets.bytes;
    end = GameQueueLine(&s_ot, s_packets.bytes, -1, -2, 30, 40,
                        50, 60, 70);
    CHECK_EQ(end == (u8 *)(line + 1), 1, "line cursor");
    CHECK_EQ(line->x0, -1, "line x0");
    CHECK_EQ(line->y1, 40, "line y1");
    CHECK_EQ(line->r0, 50, "line red");
    CHECK_EQ(line->g0, 60, "line green");
    CHECK_EQ(line->b0, 70, "line blue");
    CHECK_EQ(line->code, 0x40, "line code");
}

int main(void) {
    CheckSpriteVariants();
    CheckLine();
    CheckShadedRect();
    CheckTexturedRect();
    CheckDrawMode();

    if (s_failures != 0) {
        printf("draw packet queue tests failed: %d\n", s_failures);
        return 1;
    }
    puts("draw packet queue tests passed");
    return 0;
}
