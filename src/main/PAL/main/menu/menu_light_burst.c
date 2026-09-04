#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/render.h"

enum {
    LIGHT_BURST_MAX_LEVEL = 0x200,
};

static void DrawLightBurstFadeQuad(GameOrderingTableEntry *ot, s32 level) {
    POLY_G4 *quad = RENDER_PRIM_CURSOR_AS(POLY_G4);
    u8 shade = (u8)(level / 5);

    SetPolyG4(quad);
    SetSemiTrans(quad, 0);
    setXY4(quad, 0, 0x28, 0x13F, 0x28, 0, 0x1DF, 0x13F, 0x1DF);
    setRGB0(quad, 0, 0, 0);
    setRGB1(quad, 0, 0, 0);
    setRGB2(quad, shade, shade, shade);
    setRGB3(quad, shade, shade, shade);
    AddPrim(ot, quad);
    g_RenderState.packetCursor = quad + 1;
}

static void DrawLightBurstRays(GameOrderingTableEntry *ot, s32 level) {
    s32 i;
    u8 topShade = (u8)((level * 11) / 256);
    u8 bottomShade = (u8)((level * 75) / 256);

    for (i = 0; i < MENU_LIGHT_BURST_RAY_COUNT; i++) {
        DrawGradientLine(ot, 0x30 + i * 7, 0xAA, i * 10, 0x1E0,
                         topShade, topShade, topShade, bottomShade,
                         bottomShade, bottomShade, 0x60);
    }
}

static void DrawLightBurstBands(GameOrderingTableEntry *ot, s32 level) {
    s32 i;

    for (i = 0; i < MENU_LIGHT_BURST_RAY_COUNT; i++) {
        s32 x = g_MenuLightBurstBandX.values[i];
        s32 y = g_MenuLightBurstBandY.values[i];
        s32 width = (0xA0 - (u16)x) * 2;
        u8 shade = (u8)((((((u16)y - 0xAA) << 7) / 309 + 0x16) *
                          level) /
                         512U);

        DrawSolidRect(ot, x, y, width, 2, shade, shade, shade, 0x60);
    }
}

void DrawMenuLightBurst(s32 step) {
    GameOrderingTableEntry *ot;

    if (step == 0) {
        g_MenuLightBurstLevel = 0;
        return;
    }
    g_MenuLightBurstLevel = AddClampedMenuValue(
        g_MenuLightBurstLevel, 0, 0, LIGHT_BURST_MAX_LEVEL);
    if (step < 0) {
        g_MenuLightBurstLevel = AddClampedMenuValue(
            g_MenuLightBurstLevel, step, 0, LIGHT_BURST_MAX_LEVEL);
    }

    if (g_MenuLightBurstLevel > 0 && RENDER_OT_BASE != NULL) {
        ot = &RENDER_OT_BASE[0x2BF];
        SetDrawClipRect(ot, 0, 0, 0x140, 0x1E0);
        DrawLightBurstRays(ot, g_MenuLightBurstLevel);
        DrawLightBurstBands(ot, g_MenuLightBurstLevel);
        DrawLightBurstFadeQuad(ot, g_MenuLightBurstLevel);
        SetDrawClipRect(ot, 0x48, 0, 0x140, 0x1E0);
    }

    if (step > 0) {
        g_MenuLightBurstLevel = AddClampedMenuValue(
            g_MenuLightBurstLevel, step, 0, LIGHT_BURST_MAX_LEVEL);
    }
}
