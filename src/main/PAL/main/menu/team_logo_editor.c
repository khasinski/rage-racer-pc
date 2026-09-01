#include "game/menu_internal.h"
#include "game/menu.h"
#include "game/audio.h"
#include "game/state.h"

/* Four canvas pixels share a halfword, low nibble first. The editor can pan
 * the view, so keep the retail division rule for negative coordinates. */
static u16 *TeamLogoCanvasPixelWord(s32 x, s32 y, s32 *shift) {
    s32 adjustedX = x < 0 ? x + 3 : x;
    s32 wordColumn = adjustedX >> 2;

    *shift = (x - wordColumn * 4) * 4;
    return &g_TeamLogoCanvas.halfwords[y * 16 + wordColumn];
}

static void SetTeamLogoCanvasPixel(s32 x, s32 y, u16 colour) {
    s32 shift;
    u16 *word = TeamLogoCanvasPixelWord(x, y, &shift);
    u16 mask = (u16)(0xF << shift);

    *word = (u16)((*word & ~mask) | ((colour & 0xF) << shift));
}

static u16 GetTeamLogoCanvasPixel(s32 x, s32 y) {
    s32 shift;
    u16 *word = TeamLogoCanvasPixelWord(x, y, &shift);

    return (u16)((*word >> shift) & 0xF);
}

static void PaintTeamLogoBrush(u16 colour) {
    s32 row;
    s32 column;
    s32 x = g_TeamLogoViewX + g_TeamLogoCursorX;
    s32 y = g_TeamLogoViewY + g_TeamLogoCursorY;

    for (row = 0; row < g_TeamLogoBrushSize; row++) {
        for (column = 0; column < g_TeamLogoBrushSize; column++) {
            SetTeamLogoCanvasPixel(x + column, y + row, colour);
        }
    }
}

static void AdjustTeamLogoColour(s32 step) {
    s32 shift = g_TeamLogoColorChannel * 5;
    u16 mask = (u16)(0x1F << shift);
    u16 colour = g_TeamLogoClut[g_TeamLogoPenColor] | 0x8000;
    s32 component = (colour >> shift) & 0x1F;

    component = (component + step) & 0x1F;
    g_TeamLogoClut[g_TeamLogoPenColor] =
        (u16)((colour & ~mask) | (component << shift));
}

static s32 TeamLogoDpadRepeatsNow(void) {
    return g_TeamLogoDpadRepeatTimer == 0x14 ||
           g_TeamLogoDpadRepeatTimer == 1;
}

/* Mixing a colour: the cursor walks the editable palette slots and the three
 * five-bit channels of the selected colour. */
void EditLogoPalette(void) {
    u16 pressed = g_PadPressed;
    u16 held = g_PadHeld;

    if (pressed & (PAD_CROSS | PAD_CIRCLE)) {
        PlaySoundCue(2);
        g_TeamLogoPaletteMode = 0;
        g_TeamLogoPaintArmed = 0;
    }
    if ((pressed & PAD_SELECT) &&
        ((held & (PAD_L2 | PAD_R2 | PAD_L1 | PAD_R1)) ==
         (PAD_L2 | PAD_R2 | PAD_L1 | PAD_R1))) {
        g_TeamLogoExpertMode = g_TeamLogoExpertMode == 0;
        g_TeamLogoGuideMode = g_TeamLogoGuideModePrev;
    }

    if (TeamLogoDpadRepeatsNow()) {
        if (held & PAD_LEFT) {
            PlaySoundCue(1);
            g_TeamLogoPenColor = g_TeamLogoPenColor >= 2
                                     ? g_TeamLogoPenColor - 1
                                     : 0xF;
        }
        if (held & PAD_RIGHT) {
            PlaySoundCue(1);
            g_TeamLogoPenColor = g_TeamLogoPenColor < 0xF
                                     ? g_TeamLogoPenColor + 1
                                     : 1;
        }
    }

    if (g_TeamLogoExpertMode == 0) return;
    if (held & (PAD_R1 | PAD_R2)) {
        if (g_PadPressedRepeat & PAD_UP) {
            PlaySoundCue(4);
            AdjustTeamLogoColour(-1);
        }
        if (g_PadPressedRepeat & PAD_DOWN) {
            PlaySoundCue(4);
            AdjustTeamLogoColour(1);
        }
        return;
    }

    if (pressed & PAD_UP) {
        PlaySoundCue(1);
        g_TeamLogoColorChannel = g_TeamLogoColorChannel > 0
                                     ? g_TeamLogoColorChannel - 1
                                     : 2;
    }
    if (pressed & PAD_DOWN) {
        PlaySoundCue(1);
        g_TeamLogoColorChannel = g_TeamLogoColorChannel < 2
                                     ? g_TeamLogoColorChannel + 1
                                     : 0;
    }
}

/*
 * Drawing on the canvas: the pad moves the pen, one button lays the
 * brush down and the other rubs it out, both at whatever size the brush
 * is set to. Every plot is a nibble inside a canvas word.
 */
void EditLogoCanvas(void) {
    s32 movedHorizontally;
    u16 sampledColour;
    s32 movedVertically;
    u16 pressed = g_PadPressed;
    u16 held = g_PadHeld;

    if ((held & PAD_CIRCLE) && (g_TeamLogoPaintArmed != 0)) {
        if (pressed & PAD_CIRCLE) {
            PlaySoundCue(4);
        }
        PaintTeamLogoBrush((u16)g_TeamLogoPenColor);
    }
    if (held & PAD_SQUARE) {
        if (pressed & PAD_SQUARE) {
            PlaySoundCue(4);
        }
        PaintTeamLogoBrush(0);
    }
    if (pressed & PAD_CROSS) {
        PlaySoundCue(2);
        g_TeamLogoPaletteMode = 1;
    }
    if (pressed & PAD_TRIANGLE) {
        PlaySoundCue(2);
        switch (g_TeamLogoBrushSize) {
        case 1:
            g_TeamLogoBrushSize = 2;
            break;
        case 2:
            g_TeamLogoBrushSize = 4;
            break;
        case 4:
            g_TeamLogoBrushSize = 1;
            break;
        }
        if ((g_TeamLogoCursorX + g_TeamLogoBrushSize) >= 0x20) {
            g_TeamLogoCursorX = 0x20 - g_TeamLogoBrushSize;
        }
        if ((g_TeamLogoCursorY + g_TeamLogoBrushSize) >= 0x20) {
            g_TeamLogoCursorY = 0x20 - g_TeamLogoBrushSize;
        }
    }
    if ((held & PAD_R1) && (g_TeamLogoExpertMode != 0)) {
        if (held & PAD_L1) {
            if (pressed & PAD_UP) {
                RotateTeamLogoCw();
            }
            if (pressed & PAD_DOWN) {
                FlipTeamLogoVertical();
            }
            if (pressed & PAD_LEFT) {
                RotateTeamLogoCcw();
            }
            if (pressed & PAD_RIGHT) {
                FlipTeamLogoHorizontal();
            }
        } else if (TeamLogoDpadRepeatsNow()) {
            if (held & PAD_UP) {
                ScrollTeamLogoUp();
            }
            if (held & PAD_DOWN) {
                ScrollTeamLogoDown();
            }
            if (held & PAD_LEFT) {
                ScrollTeamLogoLeft();
            }
            if (held & PAD_RIGHT) {
                ScrollTeamLogoRight();
            }
        }
    } else {
        movedHorizontally = 0;
        if (TeamLogoDpadRepeatsNow() || (held & (PAD_L2 | PAD_L1))) {
            movedVertically = 0;
            if (held & PAD_UP) {
                if (g_TeamLogoCursorY > 0) {
                    g_TeamLogoCursorY -= 1;
                    movedVertically = 1;
                } else if (g_TeamLogoViewY > 0) {
                    g_TeamLogoViewY -= 1;
                    movedVertically = 1;
                }
            }
            if (held & PAD_DOWN) {
                if ((g_TeamLogoCursorY + g_TeamLogoBrushSize) < 0x20) {
                    g_TeamLogoCursorY += 1;
                    movedVertically = 1;
                } else if (g_TeamLogoViewY < 0x20) {
                    g_TeamLogoViewY += 1;
                    movedVertically = 1;
                }
            }
            if (held & PAD_LEFT) {
                if (g_TeamLogoCursorX > 0) {
                    g_TeamLogoCursorX -= 1;
                    movedHorizontally = 1;
                } else if (g_TeamLogoViewX > 0) {
                    g_TeamLogoViewX -= 1;
                    movedHorizontally = 1;
                }
            }
            if (held & PAD_RIGHT) {
                if ((g_TeamLogoCursorX + g_TeamLogoBrushSize) < 0x20) {
                    g_TeamLogoCursorX += 1;
                    movedHorizontally = 1;
                } else if (g_TeamLogoViewX < 0x20) {
                    g_TeamLogoViewX += 1;
                    movedHorizontally = 1;
                }
            }
            if ((held & (PAD_SQUARE | PAD_CIRCLE)) &&
                (movedHorizontally || movedVertically)) {
                PlaySoundCue(4);
            }
        }
    }
    if ((pressed & PAD_R2) && (g_TeamLogoExpertMode != 0)) {
        PlaySoundCue(4);
        sampledColour = GetTeamLogoCanvasPixel(
            g_TeamLogoViewX + g_TeamLogoCursorX,
            g_TeamLogoViewY + g_TeamLogoCursorY);
        if (sampledColour == 0) {
            sampledColour = g_TeamLogoPenColor;
        }
        g_TeamLogoPenColor = sampledColour;
    }
}

void UpdateTeamLogoCanvas(void) {
    u16 held = g_PadHeld;
    s32 repeatDelay = (held & (PAD_L2 | PAD_L1)) ? 0 : 3;

    if (held & g_TeamLogoDpadRepeatMask) {
        if (g_TeamLogoDpadRepeatTimer < 0x14 + repeatDelay) {
            g_TeamLogoDpadRepeatTimer++;
        }
    } else {
        g_TeamLogoDpadRepeatTimer = 0;
    }

    g_TeamLogoDpadRepeatMask = held &
        (PAD_UP | PAD_RIGHT | PAD_DOWN | PAD_LEFT);
    if (!(held & PAD_CIRCLE)) {
        g_TeamLogoPaintArmed = 1;
    }

    if (g_TeamLogoExpertMode != 0) {
        if (g_PadPressed & PAD_SELECT) {
            g_TeamLogoGuideModePrev = g_TeamLogoGuideMode;
            g_TeamLogoGuideMode = g_TeamLogoGuideMode < 2
                                      ? g_TeamLogoGuideMode + 1
                                      : 0;
        }
    } else {
        g_TeamLogoGuideMode = 1;
    }

    if (g_TeamLogoPaletteMode == 1) {
        EditLogoPalette();
    } else {
        EditLogoCanvas();
    }
}
