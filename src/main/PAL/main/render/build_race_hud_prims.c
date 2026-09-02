#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"

#include "rage/hud_config.h"

enum {
    HUD_FRAME_BUFFER_COUNT = 2,
    TIME_ATTACK_HUD_SPRITE_COUNT = 11,
    GRAND_PRIX_HUD_SPRITE_COUNT = 12,
    GRAND_PRIX_FINAL_BADGE_ROW = 11,
};

void BuildRaceHudPrims(s32 grandPrixMode) {
    s32 col;
    s32 row;
    s32 rowCount = grandPrixMode != 0 ? GRAND_PRIX_HUD_SPRITE_COUNT
                                      : TIME_ATTACK_HUD_SPRITE_COUNT;
    const GameSpriteDesc *descs = grandPrixMode != 0
                                      ? g_RaceHudSpriteDescsGp
                                      : g_RaceHudSpriteDescsTimeTrial;

    /* Retail stores these three packets as a tightly packed 32-bit block.
     * Native OT links are pointer-sized, so relying on the original adjacent
     * storage leaves host pointers in the GP0 command stream. Rebuild the
     * same two texture-page changes and tachometer sprite explicitly in each
     * frame context. */
    for (col = 0; col < HUD_FRAME_BUFFER_COUNT; col++) {
        RaceHudPackets *hud = &g_FrameContexts[col].layout.raceHud;
        SetDrawMode(&hud->tachometerDrawModes[0], 0, 1, 9, 0);
        BuildSpriteFromDesc(&hud->tachometerFace, &g_TachoNeedleSprite);
        SetShadeTex(&hud->tachometerFace, 0);
        SetDrawMode(&hud->tachometerDrawModes[1], 0, 1, 0xA, 0);
    }

    for (row = 0; row < rowCount; row++) {
        for (col = 0; col < HUD_FRAME_BUFFER_COUNT; col++) {
            SPRT *sprite = row < COURSE_LONG_LAPS
                ? &g_FrameContexts[col].layout.raceHud.lapTimes[row]
                : &g_FrameContexts[col].layout.raceHud.labels[
                      row - COURSE_LONG_LAPS];
            BuildSpriteFromDesc(sprite, &descs[row]);
            /* Anchor every one of them to the edge the widescreen layout
             * pushes it to. DrawRaceHud recomputes this each frame for the
             * labels it draws itself, but the split time and the race
             * position take the position built here and only change their
             * texture coordinates, so those two would otherwise sit where a
             * 4:3 screen put them. */
            sprite->x0 = (s16)HudAnchorX(descs[row].x);
            if (grandPrixMode != 0 &&
                g_GrandPrixClass == GRAND_PRIX_FINAL_CLASS_INDEX &&
                row == GRAND_PRIX_FINAL_BADGE_ROW) {
                sprite->u0 += 0xE8;
            }
        }
    }
}
