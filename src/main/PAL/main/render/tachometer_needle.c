#include "game/car.h"
#include "game/render.h"
#include "game/render_internal.h"

void BuildTachometerFace(const CarTachometerSpec *spec) {
    RaceHudPackets *hud0 = &g_FrameContexts[0].layout.raceHud;
    RaceHudPackets *hud1 = &g_FrameContexts[1].layout.raceHud;
    SPRT *prim0 = &hud0->tachometerFace;
    SPRT *prim1 = &hud1->tachometerFace;
    GameSpriteDesc sprite = g_TachoNeedleSprite;

    sprite.x = spec->faceDX + spec->needleX;
    sprite.y = spec->faceDY + spec->needleY;

    BuildSpriteFromDesc(prim0, &sprite);
    BuildSpriteFromDesc(prim1, &sprite);
    SetShadeTex(prim0, 0);
    SetShadeTex(prim1, 0);
    SetDrawMode(&hud0->tachometerDrawModes[0], 0, 1, 9, 0);
    SetDrawMode(&hud0->tachometerDrawModes[1], 0, 1, 0xA, 0);
    SetDrawMode(&hud1->tachometerDrawModes[0], 0, 1, 9, 0);
    SetDrawMode(&hud1->tachometerDrawModes[1], 0, 1, 0xA, 0);
}
