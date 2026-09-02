#include "game/prim.h"
#include "game/menu.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/input_internal.h"
#include "game/state.h"

void DrawControllerConfigScreen(void) {
    s32 selection;
    GameOrderingTableEntry *ot;
    u8 *prim;

    selection = g_PadType == PAD_TYPE_NEGCON ? g_NegconMappingIndex
                                              : g_PadMappingIndex;
    selection = ClampControllerMappingIndex(selection);
    if (g_PadErrorState != PAD_ERROR_STATE_NONE) {
        if (g_PadErrorState == PAD_ERROR_STATE_DISCONNECTED) {
            DrawProportionalText(0x3A, 0xEA, g_MsgInsertController, 0x7812);
        } else {
            DrawProportionalText(0x40, 0xEA, g_MsgControllerError, 0x7812);
        }
        return;
    }

    ot = GameSecondaryOrderingTable(51);
    prim = RENDER_PRIM_CURSOR_AS(u8);
    prim = DrawLeftArrow(
        ot, prim, 0x28, 0xE0, selection != CONTROLLER_MAPPING_FIRST);
    prim = DrawRightArrow(
        ot, prim, 0x108, 0xE0, selection != CONTROLLER_MAPPING_LAST);
    if (g_PadType == PAD_TYPE_NEGCON) {
        prim = DrawPadConfigSelector(ot, prim, 0xF0, 0x28, selection);
        prim = DrawNegconConfigDiagram(ot, prim);
        prim = GameQueueSpriteTrans(
            ot, prim, 0x10, 0x40, 0xD8, 0x10, 0, 0xA8, 0x7F40);
        prim = QueueDrawModePrim(ot, prim, 0x3F);
    } else {
        prim = DrawPadConfigSelector(ot, prim, 0xF0, 0x28, selection);
        prim = DrawPadConfigDiagram(ot, prim);
    }
    g_RenderState.packetCursor = prim;
}

void DrawNegconNeutralScreen(void) {
    GameOrderingTableEntry *ot;
    u8 *prim;

    DrawSpriteString(0x18, 0x30, g_MsgNegconUntwistedLine1, 0x7F81);
    DrawSpriteString(0x18, 0x48, g_MsgNegconUntwistedLine2, 0x7F81);
    ot = GamePrimaryOrderingTable(52);
    prim = RENDER_PRIM_CURSOR_AS(u8);
    prim = AddTilePrim(ot, prim, 0, 0x28, 0x124, 0x40, 0, 0, 0);
    g_RenderState.packetCursor =
        AddTilePrim(ot, prim, 0, 0x26, 0x125, 0x44, 0xFF, 0xFF, 0xFF);
}
