#include "game/diagnostics.h"
#include "game/prim.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_hud_internal.h"
#include "game/render.h"
#include "game/render_internal.h"

#include <stdio.h>

#include "rage/hud_config.h"

enum {
    TACHOMETER_BLEND_FRAMES = 96,
    TACHOMETER_DARK_LEVEL = 32,
    TACHOMETER_NORMAL_LEVEL = 128,
};

static s32 ClampTachometerBlend(s32 amount) {
    if (amount < 0) {
        return 0;
    }
    if (amount > TACHOMETER_BLEND_FRAMES) {
        return TACHOMETER_BLEND_FRAMES;
    }
    return amount;
}

static u8 BlendTachometerChannel(s32 from, s32 to, s32 amount) {
    return (u8)((from * (TACHOMETER_BLEND_FRAMES - amount) + to * amount) /
                TACHOMETER_BLEND_FRAMES);
}

static void SetTachometerFaceBrightness(s32 brightness) {
    g_TachoFaceR = (u8)brightness;
    g_TachoFaceG = (u8)brightness;
    g_TachoFaceB = (u8)brightness;
}

static void SetTachometerNeedleColor(POLY_F4 *needle,
                                    const CarTachometerSpec *spec,
                                    TachometerLightingMode lighting,
                                    s32 amount, GameFrameContext *frame) {
    if (lighting == TACHOMETER_LIGHTING_FADE_TO_DARK) {
        amount = ClampTachometerBlend(amount);
        SetTachometerFaceBrightness(TACHOMETER_NORMAL_LEVEL - amount);
        needle->r0 = BlendTachometerChannel(
            spec->needleColor[0], TACHOMETER_DARK_LEVEL, amount);
        needle->g0 = BlendTachometerChannel(
            spec->needleColor[1], TACHOMETER_DARK_LEVEL, amount);
        needle->b0 = BlendTachometerChannel(
            spec->needleColor[2], TACHOMETER_DARK_LEVEL, amount);
    } else if (lighting == TACHOMETER_LIGHTING_FADE_FROM_DARK) {
        amount = ClampTachometerBlend(amount - TACHOMETER_DARK_LEVEL);
        SetTachometerFaceBrightness(TACHOMETER_DARK_LEVEL + amount);
        needle->r0 = BlendTachometerChannel(
            TACHOMETER_DARK_LEVEL, spec->needleColor[0], amount);
        needle->g0 = BlendTachometerChannel(
            TACHOMETER_DARK_LEVEL, spec->needleColor[1], amount);
        needle->b0 = BlendTachometerChannel(
            TACHOMETER_DARK_LEVEL, spec->needleColor[2], amount);
        frame->layout.raceHud.tachometerFace.clut = 0x33A8;
    } else if (lighting == TACHOMETER_LIGHTING_DARK) {
        frame->layout.raceHud.tachometerFace.clut = 0x33E8;
        SetTachometerFaceBrightness(TACHOMETER_NORMAL_LEVEL);
        needle->r0 = spec->needleColorAlt[0];
        needle->g0 = spec->needleColorAlt[1];
        needle->b0 = spec->needleColorAlt[2];
    } else {
        frame->layout.raceHud.tachometerFace.clut = 0x33A8;
        SetTachometerFaceBrightness(TACHOMETER_NORMAL_LEVEL);
        needle->r0 = spec->needleColor[0];
        needle->g0 = spec->needleColor[1];
        needle->b0 = spec->needleColor[2];
    }
}

void DrawTachometer(s32 rpm, s32 flash, TachometerLightingMode lighting,
                    s32 amount) {
    const CarTachometerSpec *spec = &g_CarSpec->tachometer;
    GameFrameContext *frame = g_DrawBuffer;
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    const s32 centerX = HudRightX(spec->needleX);
    const s32 centerY = spec->needleY;
    const s32 angle =
        spec->angleMin + rpm * (spec->angleMax - spec->angleMin) / 10000;
    const s32 sine = rcos(angle);
    const s32 cosine = rsin(angle);
    POLY_F4 *needle = RENDER_PRIM_CURSOR_AS(POLY_F4);
    s16 *vertex = &needle->x0;
    u8 *next;
    TILE *shiftLight;
    s32 i;

    SetPolyF4(needle);
    for (i = 0; i < 4; i++) {
        const s32 localX = g_TachoNeedleQuad[i][0];
        const s32 localY = g_TachoNeedleQuad[i][1];
        *vertex++ = centerX + (sine * localX - cosine * localY) / 4096;
        *vertex++ = centerY + (cosine * localX + sine * localY) / 4096;
    }

    SetTachometerNeedleColor(needle, spec, lighting, amount, frame);

    if (DiagnosticsEnabled("render.tachometer_trace")) {
        printf("tacho rpm=%d angle=%d color=%02x%02x%02x "
               "quad=%d,%d/%d,%d/%d,%d/%d,%d "
               "v=%d,%d/%d,%d/%d,%d/%d,%d\n",
               rpm, angle, needle->r0, needle->g0, needle->b0,
               g_TachoNeedleQuad[0][0], g_TachoNeedleQuad[0][1],
               g_TachoNeedleQuad[1][0], g_TachoNeedleQuad[1][1],
               g_TachoNeedleQuad[2][0], g_TachoNeedleQuad[2][1],
               g_TachoNeedleQuad[3][0], g_TachoNeedleQuad[3][1], needle->x0,
               needle->y0, needle->x1, needle->y1, needle->x2, needle->y2,
               needle->x3, needle->y3);
    }
    AddPrim(ot, needle);
    next = DrawHudDigit(
        (u8 *)(needle + 1), centerX + spec->gearDigitDX,
        centerY + spec->gearDigitDY, g_PlayerCar.drive.gear, g_HudGlyphClut);
    g_RenderState.packetCursor = next;
    DrawSpeedDigits(centerX, centerY, g_PlayerCar.speed * 160 / 1168);

    frame->layout.raceHud.tachometerFace.r0 = g_TachoFaceR;
    frame->layout.raceHud.tachometerFace.g0 = g_TachoFaceG;
    frame->layout.raceHud.tachometerFace.b0 = g_TachoFaceB;
    frame->layout.raceHud.tachometerFace.x0 = HudRightX(g_TachoNeedleSprite.x);
    AddPrim(ot, &frame->layout.raceHud.tachometerDrawModes[0]);
    AddPrim(ot, &frame->layout.raceHud.tachometerFace);
    AddPrim(ot, &frame->layout.raceHud.tachometerDrawModes[1]);

    shiftLight = RENDER_PRIM_CURSOR_AS(TILE);
    SetTile(shiftLight);
    shiftLight->x0 = centerX + spec->shiftLightDX;
    shiftLight->y0 = centerY + spec->shiftLightDY;
    shiftLight->w = 0x10;
    shiftLight->h = 0x10;
    shiftLight->r0 = flash * 223 + 32;
    shiftLight->g0 = 0x20;
    shiftLight->b0 = 0x20;
    AddPrim(ot, shiftLight);
    g_RenderState.packetCursor = (u8 *)(shiftLight + 1);
}
