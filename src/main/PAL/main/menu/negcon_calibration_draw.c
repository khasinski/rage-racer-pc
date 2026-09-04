#include "game/prim.h"
#include "game/menu.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/input_internal.h"

enum {
    NEGCON_GAUGE_CENTER_Y = 230,
    NEGCON_GAUGE_LEFT = 0x94,
    NEGCON_GAUGE_RIGHT = 0xA8,
    NEGCON_PLAY_PERCENT_SCALE = 128,
};

static s32 NegconPlayGaugeHalfSpan(s32 percent) {
    return (percent * NEGCON_PLAY_PERCENT_SCALE / 100) * 2;
}

static u8 *QueueDoubleGaugeLine(GameOrderingTableEntry *ot, u8 *prim, s32 y,
                                s32 r, s32 g, s32 b) {
    prim = GameQueueLine(ot, prim, NEGCON_GAUGE_LEFT, y, NEGCON_GAUGE_RIGHT,
                         y, r, g, b);
    return GameQueueLine(ot, prim, NEGCON_GAUGE_LEFT, y + 1,
                         NEGCON_GAUGE_RIGHT, y + 1, r, g, b);
}

static u8 *QueueCalibrationArrows(GameOrderingTableEntry *ot, u8 *prim,
                                  NegconCalibrationValue value) {
    prim = DrawLeftArrow(ot, prim, 0x28, 0xE0,
                         value != NEGCON_CALIBRATION_FIRST);
    return DrawRightArrow(ot, prim, 0x108, 0xE0,
                          value != NEGCON_CALIBRATION_LAST);
}

static u8 *QueueCalibrationPanel(GameOrderingTableEntry *ot, u8 *prim) {
    prim = QueueDrawModePrim(ot, prim, 0x3F);
    prim = AddTilePrim(ot, prim, 0, 0x28, 0x124, 0x40, 0, 0, 0);
    return AddTilePrim(ot, prim, 0, 0x26, 0x125, 0x44,
                       0xFF, 0xFF, 0xFF);
}

void DrawNegconSteerPlayScreen(void) {
    GameOrderingTableEntry *ot;
    u8 *prim;
    s32 halfSpan;
    s32 upperY;
    s32 lowerY;
    s32 play = NegconCalibrationIndex(g_NegconSteerPlay);

    DrawSpriteString(0x18, 0x30, g_MsgNegconSteerPlay, 0x7F81);
    ot = GamePrimaryOrderingTable(51);
    prim = RENDER_PRIM_CURSOR_AS(u8);
    prim = QueueCalibrationArrows(ot, prim, play);
    prim = GameQueueSpriteTrans(
        ot, prim, 0x70, 0x30, 0xC, 0x18, 0x8C, 0x18, 0x7F81);
    prim = GameQueueSpriteTrans(
        ot, prim, 0x7C, 0x30, 0xC, 0x18, play * 12 + 152, 0x18, 0x7F81);
    prim = GameQueueSpriteTrans(
        ot, prim, 0x88, 0x30, 0xC, 0x18, 0x6C, 0x30, 0x7F81);
    prim = QueueCalibrationPanel(ot, prim);

    halfSpan = NegconPlayGaugeHalfSpan(g_NegconPlayPercent[play]);
    upperY = NEGCON_GAUGE_CENTER_Y - halfSpan;
    lowerY = NEGCON_GAUGE_CENTER_Y + halfSpan;
    prim = QueueDoubleGaugeLine(ot, prim, upperY, 0x20, 0x40, 0xFF);
    prim = QueueDoubleGaugeLine(ot, prim, lowerY, 0x20, 0x40, 0xFF);
    g_RenderState.packetCursor =
        QueueDoubleGaugeLine(ot, prim, NEGCON_GAUGE_CENTER_Y, 0, 0, 0);
}

void DrawNegconMaxTwistScreen(void) {
    GameOrderingTableEntry *ot;
    u8 *prim;
    s32 gaugeXOffset;
    s32 gaugeWidth;
    s32 maxTwist = NegconCalibrationIndex(g_NegconMaxTwist);

    DrawSpriteString(0x18, 0x30, g_MsgNegconMaxTwist, 0x7F81);
    ot = GamePrimaryOrderingTable(51);
    prim = RENDER_PRIM_CURSOR_AS(u8);
    prim = QueueCalibrationArrows(ot, prim, maxTwist);
    if (maxTwist == NEGCON_CALIBRATION_LAST) {
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
        maxTwist * 24,
        0x30,
        0x7F81);
    prim = GameQueueSpriteTrans(ot, prim, 0xAC, 0x30, 4, 0x18, 0x78, 0x30,
                                0x7F81);
    g_RenderState.packetCursor = QueueCalibrationPanel(ot, prim);
}
