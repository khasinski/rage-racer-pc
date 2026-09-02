#include "game/prim.h"
#include "game/render.h"

static u8 *DrawConfigArrow(GameOrderingTableEntry *ot, u8 *prim, s32 x,
                           s32 y, s32 textureU, s32 pulse) {
    prim = GameQueueSprite(
        ot, prim, x, y, 0x10, 0x20, textureU, 0xB8, 0x7F82);
    prim = QueueDrawModePrim(ot, prim, 0x39);
    if (pulse != 0) {
        u8 glow = rsin(g_SetupArrowPulse % 0x1000) / 64 - 65;

        prim = AddTilePrim(ot, prim, x, y, 0x10, 0x20, 0, glow, 0);
    }
    return prim;
}

u8 *DrawLeftArrow(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                  s32 pulse) {
    return DrawConfigArrow(ot, prim, x, y, 0x48, pulse);
}

u8 *DrawRightArrow(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                   s32 pulse) {
    return DrawConfigArrow(ot, prim, x, y, 0x58, pulse);
}

/* The framed "CONFIG n" panel: a caption, three digit cells and two nested
 * plates for each of its upper and lower halves. */
u8 *DrawPadConfigSelector(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                          s32 selection) {
    prim = GameQueueShadedSprite(
        ot, prim, x + 6, y + 8, 0x30, 0xC, 0x78, 0xC0, 0x7F40, 0x80);
    prim = QueueDrawModePrim(ot, prim, 0x3A);
    prim = GameQueueSpriteTrans(
        ot, prim, x + 18, y + 32, 8, 0x10, 0x68, 0x28, 0x7F40);
    prim = GameQueueSpriteTrans(
        ot, prim, x + 26, y + 32, 8, 0x10, selection * 8 + 80, 0x18, 0x7F40);
    prim = GameQueueSpriteTrans(
        ot, prim, x + 34, y + 32, 8, 0x10, 0x68, 0x28, 0x7F40);
    prim = QueueDrawModePrim(ot, prim, 0x5B);
    prim = AddTilePrim(ot, prim, x + 1, y + 2, 0x3A, 0x14, 0, 0, 0);
    prim = AddTilePrim(
        ot, prim, x + 2, y + 26, 0x38, 0x1A, 0xFF, 0xFF, 0xFF);
    prim = AddTilePrim(ot, prim, x + 1, y + 24, 0x3A, 0x1E, 0, 0, 0);
    return AddTilePrim(
        ot, prim, x, y, 0x3C, 0x38, 0xFF, 0xFF, 0xFF);
}
