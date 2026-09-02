#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"

#include "rage/hud_config.h"


void BuildRaceHudPrims(s32 mode) {
    s32 col;
    s32 row;
    s32 rowCount = mode != 0 ? 12 : 11;
    const GameSpriteDesc *descs = mode != 0 ? g_RaceHudSpriteDescsGp
                                            : g_RaceHudSpriteDescsTimeTrial;

    /* Retail stores these three packets as a tightly packed 32-bit block.
     * Native OT links are pointer-sized, so relying on the original adjacent
     * storage leaves host pointers in the GP0 command stream. Rebuild the
     * same two texture-page changes and tachometer sprite explicitly in each
     * frame context. */
    for (col = 0; col < 2; col++) {
        RaceHudPackets *hud = &g_FrameContexts[col].layout.raceHud;
        SetDrawMode(&hud->tachometerDrawModes[0], 0, 1, 9, 0);
        BuildSpriteFromDesc(&hud->tachometerFace, &g_TachoNeedleSprite);
        SetShadeTex(&hud->tachometerFace, 0);
        SetDrawMode(&hud->tachometerDrawModes[1], 0, 1, 0xA, 0);
    }

    for (row = 0; row < rowCount; row++) {
        for (col = 0; col < 2; col++) {
            SPRT *sprite = row < 6
                ? &g_FrameContexts[col].layout.raceHud.lapTimes[row]
                : &g_FrameContexts[col].layout.raceHud.labels[row - 6];
            BuildSpriteFromDesc(sprite, &descs[row]);
            /* Anchor every one of them to the edge the widescreen layout
             * pushes it to. DrawRaceHud recomputes this each frame for the
             * labels it draws itself, but the split time and the race
             * position take the position built here and only change their
             * texture coordinates, so those two would otherwise sit where a
             * 4:3 screen put them. */
            sprite->x0 = (s16)HudAnchorX(descs[row].x);
            if (mode != 0 &&
                g_GrandPrixClass == GRAND_PRIX_FINAL_CLASS_INDEX &&
                row == 11)
                sprite->u0 += 0xE8;
        }
    }
}
