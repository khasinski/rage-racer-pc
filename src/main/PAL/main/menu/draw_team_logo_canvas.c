#include "game/audio.h"
#include "game/menu.h"
#include "game/menu_internal.h"

static u8 LogoPulseShade(const TeamLogo *logo) {
    u32 phase = (u32)logo->colorCycle * 2u & 0xFFFu;

    return (u8)((rsin((s32)phase) / 64) - 0x41);
}

static u8 TeamLogoTexturePage(void) {
    return (u8)(((g_TeamLogoRect.y >> 4) & 0x10) |
                ((g_TeamLogoRect.x & 0x3FF) >> 6));
}

static u8 LogoColorRed(u16 color) { return (u8)((color & 0x1F) << 3); }
static u8 LogoColorGreen(u16 color) { return (u8)((color & 0x3E0) >> 2); }
static u8 LogoColorBlue(u16 color) { return (u8)((color & 0x7C00) >> 7); }

static u16 FadeLogoColor(u16 color, s32 fade) {
    s32 red = (color & 0x1F) * fade / 256;
    s32 green = ((color >> 5) & 0x1F) * fade / 256;
    s32 blue = ((color >> 10) & 0x1F) * fade / 256;

    return (u16)(0x8000 | red | (green << 5) | (blue << 10));
}

/*
 * The big canvas: its frame slides down, the brush outline blinks over the
 * cursor, and the canvas itself goes down as one zoomed textured quad.
 */
static void DrawCanvasPanel(const TeamLogo *logo, GameOrderingTableEntry *ot,
                            s32 slide) {
    s32 panelTop;
    s32 frameX;
    s32 quadLeft;
    s32 quadRight;
    s32 quadTop;
    s32 quadBottom;
    s32 zoomShortfall;
    s32 texLeft;
    s32 texTop;
    s32 texRight;
    s32 texBottom;

    if (slide < 0) {
        return;
    }
    if (slide >= 0xC) {
        slide = 0xB;
    }

    /* The frame slides down from off the top over twelve steps. */
    frameX = 0x87;
    panelTop = 0xFEC9 + slide * 35;
    DrawRectOutline(ot, (s16)frameX, (s16)panelTop, (s16)0x82, 0x104, (u8)0xB4, (u8)0xB4, (u8)0xB4,
                    (u8)0xFF);

    /* Zoomed in and not mixing a colour, the brush gets a pulsing outline. */
    if ((logo->zoom >= 0x100) && (logo->paletteMode == 0)) {
        u8 shade = LogoPulseShade(logo);
        DrawRectOutline(ot, (s16)((logo->cursorX * 4) + 0x88),
                        (s16)(panelTop + (logo->cursorY * 8) + 2),
                        (s16)(logo->brushSize * 4), (s16)(logo->brushSize * 8), 0,
                        (u8)shade, 0, (u8)0xFF);
    }

    /* Zooming in nudges the canvas a pixel left and two up inside its frame. */
    quadTop = (s16)panelTop;
    quadLeft = ((s16)frameX) - (logo->zoomSpan < 0x220);
    if (logo->zoomSpan < 0x220) {
        quadTop -= 2;
    }
    quadBottom = quadTop + 0x110;
    quadRight = quadLeft + 0x88;

    /* The view scrolls by taking a smaller window of the texture, panned by how
     * far the zoom has closed in. */
    zoomShortfall = 0x220 - logo->zoomSpan;
    texLeft =
        ((g_TeamLogoRect.x * 4) - 1) + ((zoomShortfall * logo->viewX) / 272);
    texTop = ((u8)g_TeamLogoRect.y - 1) + ((zoomShortfall * logo->viewY) / 272);
    texRight = texLeft + (logo->zoomSpan / 8);
    texBottom = texTop + (logo->zoomSpan / 8);

    SetDrawClipRect(&g_RenderState.draw, ot, (s16)0, (s16)0, (s16)0x140, (s16)0x1E0);
    GameDrawTexturedQuad(ot, (s16)quadLeft, (s16)quadTop, (s16)quadRight, (s16)quadTop,
                         (s16)quadLeft, (s16)quadBottom, (s16)quadRight, (s16)quadBottom,
                         (u8)texLeft, (u8)texTop, (u8)texRight, (u8)texTop, (u8)texLeft,
                         (u8)texBottom, (u8)texRight, (u8)texBottom, (u8)0x7F, (u8)0x7F, (u8)0x7F,
                         0x27F, 1, 0, TeamLogoTexturePage());
    SetDrawClipRect(&g_RenderState.draw, ot, (s16)(frameX + 1), (s16)(panelTop + 2), (s16)0x80, (s16)0x100);
}
/*
 * The small unzoomed preview, with the guide lines that mark the brush and
 * its row across the whole logo.
 */
static void DrawPreviewPanel(const TeamLogo *logo, GameOrderingTableEntry *ot,
                             s32 slide) {
    s32 panelTop;
    s32 viewLeft;
    s32 viewTop;
    s32 texLeft;
    s32 texTop;
    s32 clutIndex;

    if (slide < 0) {
        return;
    }
    if (slide >= 8) {
        slide = 7;
    }

    /* This one slides up from below over eight steps. */
    panelTop = 0x1FB - slide * 35;
    DrawRectOutline(ot, (s16)0x2F, (s16)panelTop, (s16)0x42, 0x84, (u8)0xB4, (u8)0xB4, (u8)0xB4,
                    (u8)0xFF);

    /* Zoomed in, the preview marks where the big panel is looking. */
    if ((logo->zoom >= 0x100) && (logo->guideMode != 0)) {
        viewLeft = (u16)((u16)logo->viewX + 0x30);
        viewTop = (u16)(panelTop + ((logo->viewY * 2) + 2));
        u8 shade = LogoPulseShade(logo);
        if (logo->guideMode == 2) {
            /* Crosshairs: both edges of the brush drawn the full height and the
             * full width of the preview. Each row of the logo is two pixels
             * here, so a row needs a pair of lines. */
            s16 top = (s16)(panelTop + 2);
            s16 bottom = (s16)(panelTop + 0x82);
            s16 column = (s16)(viewLeft + (u16)logo->cursorX);
            s32 lastRow = (logo->cursorY + logo->brushSize) - 1;
            s16 row;

            DrawLine(ot, column, top, column, bottom, (u8)shade, (u8)shade, (u8)shade, (u8)0xFF);
            column = (s16)((column + ((u16)logo->brushSize)) - 1);
            DrawLine(ot, column, top, column, bottom, (u8)shade, (u8)shade, (u8)shade, (u8)0xFF);
            for (row = (s16)(viewTop + (logo->cursorY * 2));
                 row <= (s16)(viewTop + (logo->cursorY * 2) + 1); row++) {
                DrawLine(ot, (s16)0x30, row, (s16)0x70, row, (u8)shade, (u8)shade, (u8)shade,
                         (u8)0xFF);
            }
            for (row = (s16)(viewTop + (lastRow * 2)); row <= (s16)(viewTop + (lastRow * 2) + 1);
                 row++) {
                DrawLine(ot, (s16)0x30, row, (s16)0x70, row, (u8)shade, (u8)shade, (u8)shade,
                         (u8)0xFF);
            }
        } else if (logo->brushSize == 1) {
            /* A single pixel of the logo is a two-pixel line in the preview. */
            s16 column = (s16)(viewLeft + (u16)logo->cursorX);
            s16 row = (s16)(viewTop + (logo->cursorY * 2));

            DrawLine(ot, column, row, column, (s16)(row + 1), (u8)shade, (u8)shade, (u8)shade,
                     (u8)0xFF);
        } else {
            DrawRectOutline(ot, (s16)(viewLeft + (u16)logo->cursorX),
                            (s16)(viewTop + logo->cursorY * 2), (s16)logo->brushSize,
                            (s16)(logo->brushSize * 2), (u8)shade, (u8)shade, (u8)shade,
                            (u8)0xFF);
        }
        DrawRectOutline(ot, (s16)viewLeft, (s16)viewTop, (s16)0x20, 0x40, 0, (u8)shade, 0,
                        (u8)0xFF);
    }

    /* The logo unzoomed, all 64 by 64 of it. */
    texLeft = (g_TeamLogoRect.x * 4) - 1;
    texTop = (u8)g_TeamLogoRect.y - 1;
    clutIndex = GetClut(g_TeamLogoClutRect.x, g_TeamLogoClutRect.y);
    SetDrawClipRect(&g_RenderState.draw, ot, (s16)0, (s16)0, (s16)0x140, (s16)0x1E0);
    GameDrawTexturedQuad(ot, (s16)0x2F, (s16)panelTop, (s16)0x70, (s16)panelTop, (s16)0x2F,
                         (s16)(panelTop + 0x83), (s16)0x70, (s16)(panelTop + 0x83), (u8)texLeft,
                         (u8)texTop, (u8)(texLeft + 0x41), (u8)texTop, (u8)texLeft,
                         (u8)(texTop + 0x41), (u8)(texLeft + 0x41), (u8)(texTop + 0x41), (u8)0x7F,
                         (u8)0x7F, (u8)0x7F, clutIndex & 0xFFFF, 1, 0, TeamLogoTexturePage());
    SetDrawClipRect(&g_RenderState.draw, ot, (s16)0x30, (s16)(panelTop + 2), (s16)0x40, (s16)0x80);
}

/*
 * The fifteen fixed colours, the pen well showing the mixed colour, and the
 * four button prompts, whose glyphs differ between pad and NeGcon.
 */
static void DrawSwatchStrip(const TeamLogo *logo, GameOrderingTableEntry *ot,
                            s32 slide) {
    s32 panelTop;
    s32 stripX;
    s32 wellX;
    s32 wellTop;
    s32 i;

    if (slide < 0) {
        return;
    }
    if (slide >= 6) {
        slide = 5;
    }

    stripX = 0x8A;
    panelTop = 0x1EA - slide * 30;

    /* The pen well, above the strip, outlined in the pulsing colour while a
     * colour is being mixed and in grey otherwise. */
    wellTop = (u16)(panelTop - 3);
    wellX = (u16)((logo->penColor * 8) + 0x80);
    if (logo->paletteMode == 1) {
        u8 shade = LogoPulseShade(logo);
        DrawRectOutline(ot, (s16)wellX, (s16)wellTop, (s16)0xD, 0x1A, 0, (u8)shade, 0, (u8)0xFF);
    } else {
        DrawRectOutline(ot, (s16)wellX, (s16)wellTop, (s16)0xD, 0x1A, (u8)0xB4, (u8)0xB4, (u8)0xB4,
                        (u8)0xFF);
    }
    DrawSolidRect(ot, (s16)(wellX + 1), (s16)(wellTop + 2), (s16)0xB, (s16)0x16,
                  LogoColorRed(g_TeamLogoClut[logo->penColor]),
                  LogoColorGreen(g_TeamLogoClut[logo->penColor]),
                  LogoColorBlue(g_TeamLogoClut[logo->penColor]), (u8)0xFF);

    /* The fifteen fixed colours, eight pixels apart along the strip. */
    for (i = 0; i < 15; i++) {
        DrawSolidRect(ot, (s16)(stripX + 1 + i * 8), (s16)(panelTop + 2), (s16)8, (s16)0x10,
                      LogoColorRed(logo->swatches[i]), LogoColorGreen(logo->swatches[i]),
                      LogoColorBlue(logo->swatches[i]), (u8)0xFF);
    }
    DrawRectOutline(ot, (s16)stripX, (s16)panelTop, (s16)0x7A, 0x14, (u8)0xB4, (u8)0xB4, (u8)0xB4,
                    (u8)0xFF);

    /* Four button prompts under the strip: the button's own glyph, and the
     * caption strip that goes with it. The glyphs sit in different places in
     * VRAM for a pad and for a NeGcon. */
    for (i = 0; i < 4; i++) {
        s32 promptX = stripX + (i * 0x28) - 0xF;
        s32 glyphU;
        s32 glyphV;
        u16 glyphClut;
        s32 glyphTpage;

        if (g_PadType == PAD_TYPE_NEGCON) {
            glyphU = (i * 0xC) - 0x30;
            glyphV = 0;
            glyphClut = 0x233;
            glyphTpage = 0x1E;
        } else {
            glyphU = (i * 0xC) + 0x60;
            glyphV = 0x58;
            glyphClut = 0x1F6;
            glyphTpage = 0x1C;
        }
        DrawSprite(ot, (s16)(promptX + 0x13), (s16)(panelTop + 0x22), (s16)0xC, (s16)0x18,
                   (u8)glyphU, (u8)glyphV, 0, 0, 0, glyphClut, 1, 0, glyphTpage);
        DrawSprite(ot, (s16)promptX, (s16)(panelTop + 0x1C), (s16)0x22, (s16)0x32, (u8)(i * 0x24),
                   (u8)0xC0, 0, 0, 0, 0x1F5, 1, 0, 0x1D);
    }
}

/*
 * The caption that slides in from the left edge.
 */
static void DrawEditorHint(GameOrderingTableEntry *ot, s32 slide) {
    if (slide < 0) {
        return;
    }
    if (slide >= 7) {
        slide = 6;
    }
    DrawSprite(ot, (s16)((s32)(((u32)slide * 0x250) >> 5) + 0xFFA1), (s16)0xC0, (s16)0x61,
               (s16)0x32, (u8)0x90, (u8)0xC0, 0, 0, 0, 0x1F5, 1, 0, 0x1D);
}

/*
 * Expert mode's three colour channels: a numeric readout and a bar for each
 * of red, green and blue.
 */
static void DrawChannelSliders(const TeamLogo *logo, GameOrderingTableEntry *ot,
                               s32 slide) {
    /* Red, green and blue, one slider each, 0x30 apart down the screen. */
    static const u8 glyphU[3] = {0xD8, 0x80, 0x58};
    static const u8 barRed[3] = {0xC0, 0, 0};
    static const u8 barGreen[3] = {0, 0xC0, 0};
    static const u8 barBlue[3] = {0, 0, 0xC0};
    const s32 top = 0xC8;
    s32 sliderX;
    s32 i;

    if (slide < 0) {
        return;
    }
    if (slide >= 6) {
        slide = 5;
    }

    /* The column slides in from the right edge over six steps. */
    sliderX = 0x140 - slide * 10;

    /* While a colour is being mixed, the channel being edited is ringed. */
    if (logo->paletteMode == 1) {
        u8 shade = LogoPulseShade(logo);

        DrawRectOutline(ot, (s16)sliderX, (s16)((logo->colorChannel * 0x30) + 0xD9), (s16)0x12,
                        0x15, 0, (u8)shade, 0, (u8)0xFF);
    }

    /* The three readouts, to the left of the column. */
    for (i = 0; i < 3; i++) {
        GameDrawNumber((s16)(sliderX - 0x3F), (s16)(top + (i * 0x30) + 0x14),
                       DRAW_NUMBER_LARGE_DIGITS | DRAW_NUMBER_TEN_DIGIT_FIELD,
                       (g_TeamLogoClut[logo->penColor] >> (i * 5)) & 0x1F,
                       (u8)0x7F, (u8)0x7F, (u8)0x7F, 0x244, 0x20);
    }

    /* Then the three wells, their midlines, their letters and their bars, each
     * kind drawn for all three channels before the next, which is the order the
     * packets have to reach the ordering table in. */
    for (i = 0; i < 3; i++) {
        DrawRectOutline(ot, (s16)sliderX, (s16)(top + (i * 0x30)), (s16)0x12, 0x26, (u8)0xB4,
                        (u8)0xB4, (u8)0xB4, (u8)0xFF);
    }
    for (i = 0; i < 3; i++) {
        s32 midline = top + (i * 0x30) + 0x11;

        DrawLine(ot, (s16)(sliderX + 1), (s16)midline, (s16)(sliderX + 0x11), (s16)midline,
                 (u8)0xB4, (u8)0xB4, (u8)0xB4, (u8)0xFF);
        DrawLine(ot, (s16)(sliderX + 1), (s16)(midline + 1), (s16)(sliderX + 0x11),
                 (s16)(midline + 1), (u8)0xB4, (u8)0xB4, (u8)0xB4, (u8)0xFF);
    }
    for (i = 0; i < 3; i++) {
        DrawSprite(ot, (s16)(sliderX + 5), (s16)(top + (i * 0x30) + 2), (s16)8, (s16)0x10,
                   glyphU[i], (u8)0x18, 0, 0, 0, 0x244, 1, 1, 0x5B);
    }
    for (i = 0; i < 3; i++) {
        DrawSolidRect(ot, (s16)(sliderX + 1), (s16)(top + (i * 0x30) + 2), (s16)0x10, (s16)0x10,
                      barRed[i], barGreen[i], barBlue[i], (u8)0xFF);
        DrawSolidRect(ot, (s16)(sliderX + 1), (s16)(top + (i * 0x30) + 0x14), (s16)0x10, (s16)0x10,
                      0, 0, 0, (u8)0xFF);
    }
}

/*
 * Colour zero cycles through the spectrum on its own, and the whole palette is
 * then dimmed by the fade level into the second CLUT the panels draw with.
 */
static void AnimateLogoClut(TeamLogo *logo) {
    u32 phase = (u32)logo->colorCycle;
    s32 fade;
    s32 blue;
    s32 i;

    g_TeamLogoClut[0] = 0x8000;
    g_TeamLogoClut[0] |= ((rsin((s32)(phase & 0xFFFu)) / 128) + 0x20) >> 3;
    g_TeamLogoClut[0] |= (((rsin((s32)((phase + 0x55u) & 0xFFFu)) / 128) + 0x20) >> 3)
                         << 5;
    blue = rsin((s32)((phase + 0xAAu) & 0xFFFu));
    if (blue < 0) {
        blue += 0x7F;
    }
    g_TeamLogoClut[0] |= (((blue >> 7) + 0x20) >> 3) << 10;
    logo->colorCycle = (s32)(phase + 0x20u);

    fade = logo->fade;
    for (i = 0; i < 16; i++) {
        logo->fadedClut[i] = FadeLogoColor(g_TeamLogoClut[i], fade);
    }
    /* The colour strip shows the live palette, including colours the editor
     * has just mixed and the four background colours of a sample. */
    for (i = 1; i < 16; i++) {
        logo->swatches[i - 1] = g_TeamLogoClut[i];
    }
}

void DrawTeamLogoCanvas(TeamLogo *logo, s32 panelStep, s32 editorStep) {
    GameOrderingTableEntry *ot;

    ot = RENDER_OT_BASE;
    if (panelStep == 0) {
        logo->panelStep = 0;
        logo->editorStep = 0;
        return;
    }

    AnimateLogoClut(logo);
    LoadImage(&g_TeamLogoRect, &g_TeamLogoCanvas);
    LoadImage(&g_TeamLogoClutRect, g_TeamLogoClut);
    LoadImage(&logo->fadedClutRect, logo->fadedClut);
    logo->panelStep =
        AddClampedMenuValue(logo->panelStep,
                            panelStep < 0 ? panelStep : 0, 0, 0x19);
    logo->editorStep =
        AddClampedMenuValue(logo->editorStep,
                            editorStep < 0 ? editorStep : 0, 0, 0x10);
    DrawCanvasPanel(logo, ot, logo->panelStep - 0xA);

    DrawPreviewPanel(logo, ot, logo->panelStep - 0xE);

    DrawSwatchStrip(logo, ot, logo->editorStep - 8);

    DrawEditorHint(ot, logo->editorStep - 7);

    if (logo->expertMode != 0) {
        DrawChannelSliders(logo, ot, logo->editorStep - 8);
    }

    logo->panelStep =
        AddClampedMenuValue(logo->panelStep,
                            panelStep > 0 ? panelStep : 0, 0, 0x19);
    logo->editorStep =
        AddClampedMenuValue(logo->editorStep,
                            editorStep > 0 ? editorStep : 0, 0, 0x10);
}

void RampTeamLogoCanvas(TeamLogo *logo, s32 stepA, s32 stepB) {
    logo->fade =
        AddClampedMenuValue(logo->fade, stepA, 0x40, 0x100);
    logo->zoom =
        AddClampedMenuValue(logo->zoom, stepB, 0, 0x100);
    logo->zoomSpan = 0x220 - ((logo->zoom * 17) / 16);
}
