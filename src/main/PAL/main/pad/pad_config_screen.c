#include "game/prim.h"
#include "game/render.h"
#include "game/state.h"
#include "game/input_internal.h"


/*
 * The five action captions, each a strip of glyphs lifted from one texture row
 * and placed in the slot `labelRow` assigns it - slot 3 carries two words, so
 * six sprites cover five slots - followed by the black plate and white frame
 * behind each slot and one DR_MODE packet to close the run.
 */
u8 *DrawPadConfigLabels(GameOrderingTableEntry *ot, u8 *prim, u8 *labelRow) {
    u8 k;
    s32 i;

    k = labelRow[0];
    prim = GameQueueSpriteTrans(
        ot, prim, g_PadLabelSlots[k].vx + 4, g_PadLabelSlots[k].vy + 8, 0x40, 0x10, 0x40,
        0xBC, 0x7F40);
    k = labelRow[1];
    prim = GameQueueSpriteTrans(
        ot, prim, g_PadLabelSlots[k].vx + 22, g_PadLabelSlots[k].vy + 8, 0x1C, 0x10, 0x84,
        0xBC, 0x7F40);
    k = labelRow[2];
    prim = GameQueueSpriteTrans(
        ot, prim, g_PadLabelSlots[k].vx + 18, g_PadLabelSlots[k].vy + 8, 0x28, 0x10, 0x90,
        0xAC, 0x7F40);
    k = labelRow[3];
    prim = GameQueueSpriteTrans(
        ot, prim, g_PadLabelSlots[k].vx + 9, g_PadLabelSlots[k].vy + 8, 0x18, 0x10, 0x90,
        0xAC, 0x7F40);
    k = labelRow[3];
    prim = GameQueueSpriteTrans(
        ot, prim, g_PadLabelSlots[k].vx + 33, g_PadLabelSlots[k].vy + 8, 0x20, 0x10, 0xB8,
        0xAC, 0x7F40);
    k = labelRow[4];
    prim = GameQueueSpriteTrans(
        ot, prim, g_PadLabelSlots[k].vx + 5, g_PadLabelSlots[k].vy + 8, 0x40, 0x10, 0,
        0xBC, 0x7F40);
    for (i = 0; i < 5; i++) {
        k = labelRow[i];
        prim = AddTilePrim(
            ot, prim, g_PadLabelSlots[k].vx + 1, g_PadLabelSlots[k].vy + 2, 0x46,
            0x1C, 0, 0, 0);
        k = labelRow[i];
        prim = AddTilePrim(
            ot, prim, g_PadLabelSlots[k].vx, g_PadLabelSlots[k].vy, 0x48, 0x20,
            0xFF, 0xFF, 0xFF);
    }
    return QueueDrawModePrim(ot, prim, 0x3B);
}


/*
 * The five green callout lines joining each action label to its button: one
 * vertical drop from the label, then a two-pixel-thick horizontal run to the
 * button. Suppressed while the panel is still sliding.
 */
u8 *DrawPadConfigCallouts(GameOrderingTableEntry *ot, u8 *prim, u8 *labelRow,
                          u8 *buttonRow) {
    s32 i;

    if (g_ControllerSceneAngleY > -16 && g_ControllerSceneAngleY < 16) {
        for (i = 0; i < 5; i++) {
            DVec *lp = &g_PadCalloutLabelPoints[labelRow[i]];
            DVec *bp = &g_PadCalloutButtonPoints[buttonRow[i]];

            prim = GameQueueLine(
                ot, prim, lp->vx, lp->vy, lp->vx, bp->vy, 0x20, 0xFF, 0x20);
            prim = GameQueueLine(
                ot, prim, lp->vx, bp->vy, bp->vx, bp->vy, 0x20, 0xFF, 0x20);
            prim = GameQueueLine(
                ot, prim, lp->vx, bp->vy - 1, bp->vx, bp->vy - 1, 0x20, 0xFF, 0x20);
        }
    }
    return prim;
}


/* One whole standard-pad diagram for the current selection: the five action
 * labels, then the five callout lines from each label to its button. */
u8 *DrawPadConfigDiagram(GameOrderingTableEntry *ot, u8 *prim) {
    prim = DrawPadConfigLabels(ot, prim, &g_PadConfigLabelRows[g_PadMappingIndex * 5]);
    return DrawPadConfigCallouts(
        ot, prim, &g_PadConfigLabelRows[g_PadMappingIndex * 5], &g_PadConfigButtonRows[g_PadMappingIndex * 5]);
}


/* One whole NeGcon diagram for the current selection: labels, then callouts. */
u8 *DrawNegconConfigDiagram(GameOrderingTableEntry *ot, u8 *prim) {
    prim = DrawPadConfigLabels(ot, prim, &g_NegconConfigLabelRows[g_NegconMappingIndex * 5]);
    return DrawPadConfigCallouts(
        ot, prim, &g_NegconConfigLabelRows[g_NegconMappingIndex * 5], &g_NegconConfigButtonRows[g_NegconMappingIndex * 5]);
}
