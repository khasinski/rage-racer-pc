#include "common.h"
#include "game/car.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track.h"
#include "psyq/gpu.h"

void BuildTachoNeedleQuad(void) {
    CarTachometerSpec *spec = &g_CarSpec->tachometer;
    RaceHudPackets *hud0 = &g_FrameContexts[0].layout.raceHud;
    RaceHudPackets *hud1 = &g_FrameContexts[1].layout.raceHud;
    SPRT *prim0 = &hud0->tachometerFace;
    SPRT *prim1 = &hud1->tachometerFace;
    GameSpriteDesc *src = &g_TachoNeedleSprite;

    g_TachoNeedleQuad[0][0] = -spec->needleQuad[3];
    g_TachoNeedleQuad[0][1] = spec->needleQuad[2];
    g_TachoNeedleQuad[1][0] = -spec->needleQuad[1];
    g_TachoNeedleQuad[1][1] = -spec->needleQuad[0];
    g_TachoNeedleQuad[2][0] = spec->needleQuad[3];
    g_TachoNeedleQuad[2][1] = spec->needleQuad[2];
    g_TachoNeedleQuad[3][0] = spec->needleQuad[1];
    g_TachoNeedleQuad[3][1] = -spec->needleQuad[0];

    src->x = spec->faceDX + spec->needleX;
    src->y = spec->faceDY + spec->needleY;

    BuildSpriteFromDesc(prim0, src);
    BuildSpriteFromDesc(prim1, src);
    SetShadeTex(prim0, 0);
    SetShadeTex(prim1, 0);
    SetDrawMode(&hud0->tachometerDrawModes[0], 0, 1, 9, 0);
    SetDrawMode(&hud0->tachometerDrawModes[1], 0, 1, 0xA, 0);
    SetDrawMode(&hud1->tachometerDrawModes[0], 0, 1, 9, 0);
    SetDrawMode(&hud1->tachometerDrawModes[1], 0, 1, 0xA, 0);
}
