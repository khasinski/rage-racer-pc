#include "game/prim.h"
#include "game/render.h"
#include "game/state.h"
#include "game/input_internal.h"


typedef struct PadLabelSpriteSpec {
    u8 action;
    u8 xOffset;
    u8 width;
    u8 textureU;
    u8 textureV;
} PadLabelSpriteSpec;

static const PadLabelSpriteSpec s_labelSpriteSpecs[] = {
    {0, 4, 0x40, 0x40, 0xBC},
    {1, 22, 0x1C, 0x84, 0xBC},
    {2, 18, 0x28, 0x90, 0xAC},
    {3, 9, 0x18, 0x90, 0xAC},
    {3, 33, 0x20, 0xB8, 0xAC},
    {4, 5, 0x40, 0, 0xBC},
};

/* Five action captions. Action three consists of two texture strips, hence
 * six sprite descriptions for five framed label slots. */
u8 *DrawPadConfigLabels(GameOrderingTableEntry *ot, u8 *prim,
                        const u8 *labelRow) {
    s32 i;

    for (i = 0;
         i < (s32)(sizeof(s_labelSpriteSpecs) / sizeof(s_labelSpriteSpecs[0]));
         i++) {
        const PadLabelSpriteSpec *spec = &s_labelSpriteSpecs[i];
        const DVec *slot = &g_PadLabelSlots[labelRow[spec->action]];

        prim = GameQueueSpriteTrans(
            ot,
            prim,
            slot->vx + spec->xOffset,
            slot->vy + 8,
            spec->width,
            0x10,
            spec->textureU,
            spec->textureV,
            0x7F40);
    }
    for (i = 0; i < 5; i++) {
        const DVec *slot = &g_PadLabelSlots[labelRow[i]];

        prim = AddTilePrim(
            ot, prim, slot->vx + 1, slot->vy + 2, 0x46, 0x1C, 0, 0, 0);
        prim = AddTilePrim(
            ot, prim, slot->vx, slot->vy, 0x48, 0x20, 0xFF, 0xFF, 0xFF);
    }
    return QueueDrawModePrim(ot, prim, 0x3B);
}


/*
 * The five green callout lines joining each action label to its button: one
 * vertical drop from the label, then a two-pixel-thick horizontal run to the
 * button. Suppressed while the panel is still sliding.
 */
u8 *DrawPadConfigCallouts(GameOrderingTableEntry *ot, u8 *prim,
                          const u8 *labelRow, const u8 *buttonRow) {
    s32 i;

    if (g_ControllerSceneAngleY > -16 && g_ControllerSceneAngleY < 16) {
        for (i = 0; i < 5; i++) {
            const DVec *labelPoint = &g_PadCalloutLabelPoints[labelRow[i]];
            const DVec *buttonPoint = &g_PadCalloutButtonPoints[buttonRow[i]];

            prim = GameQueueLine(
                ot,
                prim,
                labelPoint->vx,
                labelPoint->vy,
                labelPoint->vx,
                buttonPoint->vy,
                0x20,
                0xFF,
                0x20);
            prim = GameQueueLine(
                ot,
                prim,
                labelPoint->vx,
                buttonPoint->vy,
                buttonPoint->vx,
                buttonPoint->vy,
                0x20,
                0xFF,
                0x20);
            prim = GameQueueLine(
                ot,
                prim,
                labelPoint->vx,
                buttonPoint->vy - 1,
                buttonPoint->vx,
                buttonPoint->vy - 1,
                0x20,
                0xFF,
                0x20);
        }
    }
    return prim;
}


/* One whole standard-pad diagram for the current selection: the five action
 * labels, then the five callout lines from each label to its button. */
u8 *DrawPadConfigDiagram(GameOrderingTableEntry *ot, u8 *prim) {
    const s32 mapping = ClampControllerMappingIndex(g_PadMappingIndex);
    const u8 *labelRow = &g_PadConfigLabelRows[mapping * 5];
    const u8 *buttonRow = &g_PadConfigButtonRows[mapping * 5];

    prim = DrawPadConfigLabels(ot, prim, labelRow);
    return DrawPadConfigCallouts(
        ot, prim, labelRow, buttonRow);
}


/* One whole NeGcon diagram for the current selection: labels, then callouts. */
u8 *DrawNegconConfigDiagram(GameOrderingTableEntry *ot, u8 *prim) {
    const s32 mapping = ClampControllerMappingIndex(g_NegconMappingIndex);
    const u8 *labelRow = &g_NegconConfigLabelRows[mapping * 5];
    const u8 *buttonRow = &g_NegconConfigButtonRows[mapping * 5];

    prim = DrawPadConfigLabels(ot, prim, labelRow);
    return DrawPadConfigCallouts(
        ot, prim, labelRow, buttonRow);
}
