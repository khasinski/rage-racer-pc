#include <stdio.h>
#include <string.h>

#include "game/prim.h"
#include "game/render.h"

s32 g_SetupArrowPulse;

static s32 s_spriteU[4];
static s32 s_spriteCount;
static s32 s_transU[4];
static s32 s_transCount;
static s32 s_tileGreen;
static s32 s_tileCount;
static s32 s_modePages[4];
static s32 s_modeCount;
static s32 s_shade;
static s32 s_shadedCount;
static s32 s_failures;

#define CHECK(condition)                                                                  \
    do {                                                                                  \
        if (!(condition)) {                                                               \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition); \
            s_failures++;                                                                 \
        }                                                                                 \
    } while (0)

int rsin(int angle) {
    CHECK(angle == 0x123);
    return 4096;
}

u8 *GameQueueSprite(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                    s32 width, s32 height, s32 u, s32 v, s32 clut) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)v;
    (void)clut;
    s_spriteU[s_spriteCount++] = u;
    return prim + 1;
}

u8 *GameQueueSpriteTrans(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                         s32 width, s32 height, s32 u, s32 v, s32 clut) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)v;
    (void)clut;
    s_transU[s_transCount++] = u;
    return prim + 1;
}

u8 *GameQueueShadedSprite(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                          s32 width, s32 height, s32 u, s32 v, s32 clut,
                          s32 shade) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)u;
    (void)v;
    (void)clut;
    s_shadedCount++;
    s_shade = shade;
    return prim + 1;
}

u8 *AddTilePrim(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                s32 width, s32 height, s32 r, s32 g, s32 b) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)r;
    (void)b;
    s_tileGreen = g;
    s_tileCount++;
    return prim + 1;
}

u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *prim, s32 tpage) {
    (void)ot;
    s_modePages[s_modeCount++] = tpage;
    return prim + 1;
}

static void Reset(void) {
    memset(s_spriteU, 0, sizeof(s_spriteU));
    memset(s_transU, 0, sizeof(s_transU));
    memset(s_modePages, 0, sizeof(s_modePages));
    s_spriteCount = 0;
    s_transCount = 0;
    s_tileGreen = -1;
    s_tileCount = 0;
    s_modeCount = 0;
    s_shade = -1;
    s_shadedCount = 0;
}

static void TestArrows(void) {
    u8 packets[8];
    GameOrderingTableEntry ot;

    Reset();
    CHECK(DrawLeftArrow(&ot, packets, 10, 20, 0) == packets + 2);
    CHECK(s_spriteCount == 1 && s_spriteU[0] == 0x48);
    CHECK(s_modeCount == 1 && s_modePages[0] == 0x39);
    CHECK(s_tileCount == 0);

    Reset();
    g_SetupArrowPulse = 0x1123;
    CHECK(DrawRightArrow(&ot, packets, 30, 40, 1) == packets + 3);
    CHECK(s_spriteCount == 1 && s_spriteU[0] == 0x58);
    CHECK(s_tileCount == 1 && s_tileGreen == (u8)-1);
}

static void TestSelector(void) {
    u8 packets[16];
    GameOrderingTableEntry ot;

    Reset();
    CHECK(DrawPadConfigSelector(&ot, packets, 100, 50, 6) == packets + 10);
    CHECK(s_shadedCount == 1 && s_shade == 0x80);
    CHECK(s_transCount == 3);
    CHECK(s_transU[0] == 0x68 && s_transU[1] == 128 && s_transU[2] == 0x68);
    CHECK(s_modeCount == 2);
    CHECK(s_modePages[0] == 0x3A && s_modePages[1] == 0x5B);
    CHECK(s_tileCount == 4);
}

int main(void) {
    TestArrows();
    TestSelector();
    return s_failures != 0;
}
