#include "game/prim.h"
#include "game/menu.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/input_internal.h"

enum {
    NEGCON_GAUGE_CENTER_Y = 230,
};

void DrawNegconSteerPlayScreen(void) {
    GameOrderingTableEntry *ot;
    u8 *prim;
    s32 halfSpan;
    s32 upperY;
    s32 lowerY;

    DrawSpriteString(0x18, 0x30, g_MsgNegconSteerPlay, 0x7F81);
    ot = GamePrimaryOrderingTable(51);
    prim = RENDER_PRIM_CURSOR_AS(u8);
    prim = DrawLeftArrow(
        ot, prim, 0x28, 0xE0,
        g_NegconSteerPlay != NEGCON_CALIBRATION_FIRST);
    prim = DrawRightArrow(
        ot, prim, 0x108, 0xE0,
        g_NegconSteerPlay != NEGCON_CALIBRATION_LAST);
    prim = GameQueueSpriteTrans(
        ot, prim, 0x70, 0x30, 0xC, 0x18, 0x8C, 0x18, 0x7F81);
    prim = GameQueueSpriteTrans(
        ot, prim, 0x7C, 0x30, 0xC, 0x18, g_NegconSteerPlay * 12 + 152, 0x18, 0x7F81);
    prim = GameQueueSpriteTrans(
        ot, prim, 0x88, 0x30, 0xC, 0x18, 0x6C, 0x30, 0x7F81);
    prim = QueueDrawModePrim(ot, prim, 0x3F);
    prim = AddTilePrim(ot, prim, 0, 0x28, 0x124, 0x40, 0, 0, 0);
    prim = AddTilePrim(ot, prim, 0, 0x26, 0x125, 0x44, 0xFF, 0xFF, 0xFF);

    halfSpan = ((g_NegconPlayPercent[g_NegconSteerPlay] << 7) / 100) * 2;
    upperY = NEGCON_GAUGE_CENTER_Y - halfSpan;
    lowerY = NEGCON_GAUGE_CENTER_Y + halfSpan;
    prim = GameQueueLine(ot, prim, 0x94, upperY, 0xA8, upperY, 0x20, 0x40, 0xFF);
    prim = GameQueueLine(ot, prim, 0x94, upperY + 1, 0xA8, upperY + 1, 0x20, 0x40, 0xFF);
    prim = GameQueueLine(ot, prim, 0x94, lowerY, 0xA8, lowerY, 0x20, 0x40, 0xFF);
    prim = GameQueueLine(ot, prim, 0x94, lowerY + 1, 0xA8, lowerY + 1, 0x20, 0x40, 0xFF);
    prim = GameQueueLine(
        ot, prim, 0x94, NEGCON_GAUGE_CENTER_Y, 0xA8, NEGCON_GAUGE_CENTER_Y, 0, 0, 0);
    g_RenderState.packetCursor = GameQueueLine(
        ot, prim, 0x94, NEGCON_GAUGE_CENTER_Y + 1, 0xA8, NEGCON_GAUGE_CENTER_Y + 1, 0, 0, 0);
}

void DrawNegconMaxTwistScreen(void) {
    GameOrderingTableEntry *ot;
    u8 *prim;
    s32 gaugeXOffset;
    s32 gaugeWidth;

    DrawSpriteString(0x18, 0x30, g_MsgNegconMaxTwist, 0x7F81);
    ot = GamePrimaryOrderingTable(51);
    prim = RENDER_PRIM_CURSOR_AS(u8);
    prim = DrawLeftArrow(
        ot, prim, 0x28, 0xE0,
        g_NegconMaxTwist != NEGCON_CALIBRATION_FIRST);
    prim = DrawRightArrow(
        ot, prim, 0x108, 0xE0,
        g_NegconMaxTwist != NEGCON_CALIBRATION_LAST);
    if (g_NegconMaxTwist == NEGCON_CALIBRATION_LAST) {
        gaugeXOffset = 0;
        gaugeWidth = 0x24;
    } else {
        gaugeXOffset = 0xC;
        gaugeWidth = 0x18;
    }
    prim = GameQueueSpriteTrans(
        ot,
        prim,
        gaugeXOffset + 0x88,
        0x30,
        gaugeWidth,
        0x18,
        g_NegconMaxTwist * 24,
        0x30,
        0x7F81);
    prim = GameQueueSpriteTrans(ot, prim, 0xAC, 0x30, 4, 0x18, 0x78, 0x30, 0x7F81);
    prim = QueueDrawModePrim(ot, prim, 0x3F);
    prim = AddTilePrim(ot, prim, 0, 0x28, 0x124, 0x40, 0, 0, 0);
    g_RenderState.packetCursor =
        AddTilePrim(ot, prim, 0, 0x26, 0x125, 0x44, 0xFF, 0xFF, 0xFF);
}
