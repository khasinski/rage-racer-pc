#include "game/menu_internal.h"
#include "game/menu.h"
#include "game/audio.h"
#include "game/state.h"

/* The pen colour, reached as the slot it names. */
static TeamLogoColorSlot *GetTeamLogoPenSlot(void) {
    TeamLogoColorAddress address;

    address.index = &g_TeamLogoPenColor;
    return address.slot;
}

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

/*
 * Mixing a colour: the cursor walks the sixteen palette slots and the
 * three channels of the one it is on, and the shoulder buttons take the
 * selected channel up and down.
 */
void EditLogoPalette(void) {
    u32 blueDown;
    s32 blueUp;
    s32 brighter;
    u16 *clutEntry;
    u16 colour;
    s32 darker;
    s32 greenUp;
    s32 nextChannel;
    s32 nextSlot;
    u16 opaqueColour;
    s32 prevChannel;
    s32 prevSlot;
    u32 redDown;
    s32 redUp;
    u32 slotValue;

    u16 *input = &g_PadPressed;

    if (*input & 0x60) {
        PlaySoundCue(2);
        g_TeamLogoPaletteMode = 0;
        g_TeamLogoPaintArmed = 0;
    }
    {
        s32 mask = 0xF;

        if ((*input & 0x100) &&
            ((g_PadHeld & mask) == mask)) {
            g_TeamLogoExpertMode = g_TeamLogoExpertMode == 0;
            g_TeamLogoGuideMode = g_TeamLogoGuideModePrev;
        }
    }
    if ((g_TeamLogoDpadRepeatTimer == 0x14) || (g_TeamLogoDpadRepeatTimer == 1)) {
        if (g_PadHeld & PAD_LEFT) {
            TeamLogoColorSlot output;
            s32 selected;

            PlaySoundCue(1);
            selected = g_TeamLogoPenColor;
            prevSlot = 0xF;
            if (selected >= 2) {
                prevSlot = selected - 1;
            }
            output.value = prevSlot;
            g_TeamLogoPenColor = output.value;
        }
        if (g_PadHeld & PAD_RIGHT) {
            TeamLogoColorSlot output;
            s32 selected;

            PlaySoundCue(1);
            selected = g_TeamLogoPenColor;
            nextSlot = 1;
            if (selected < 0xF) {
                nextSlot = selected + 1;
            }
            output.value = nextSlot;
            g_TeamLogoPenColor = output.value;
        }
    }
    if (g_TeamLogoExpertMode != 0) {
        if (g_PadHeld & (PAD_R1 | PAD_R2)) {
            if (g_PadPressedRepeat & PAD_UP) {
                PlaySoundCue(4);
                slotValue = g_TeamLogoPenColor;
                clutEntry = g_TeamLogoClut + slotValue;
                opaqueColour = *clutEntry | 0x8000;
                *clutEntry = opaqueColour;
                colour = opaqueColour;
                switch (g_TeamLogoColorChannel) {
                case 0:
                    redUp = opaqueColour & 0x1F;
                    if (redUp == 0) {
                        darker = 0x1F;
                    } else {
                        darker = redUp - 1;
                    }
                    g_TeamLogoClut[g_TeamLogoPenColor] =
                        darker |
                        (g_TeamLogoClut[g_TeamLogoPenColor] & 0xFFE0);
                    break;
                case 1:
                    greenUp = (colour >> 5) & 0x1F;
                    if (greenUp != 0) {
                        darker = (greenUp * 32) - 0x20;
                    } else {
                        darker = 0x3E0;
                    }
                    g_TeamLogoClut[g_TeamLogoPenColor] =
                        darker |
                        (g_TeamLogoClut[g_TeamLogoPenColor] & 0xFC1F);
                    break;
                case 2:
                    blueUp = (colour >> 0xA) & 0x1F;
                    if (blueUp != 0) {
                        darker = (blueUp << 0xA) - 0x400;
                    } else {
                        darker = 0x7C00;
                    }
                    g_TeamLogoClut[g_TeamLogoPenColor] =
                        darker |
                        (g_TeamLogoClut[g_TeamLogoPenColor] & 0x83FF);
                    break;
                default:
                    break;
                }
            }
            if (g_PadPressedRepeat & PAD_DOWN) {
                PlaySoundCue(4);
                slotValue = g_TeamLogoPenColor;
                clutEntry = g_TeamLogoClut + slotValue;
                opaqueColour = *clutEntry | 0x8000;
                *clutEntry = opaqueColour;
                colour = opaqueColour;
                switch (g_TeamLogoColorChannel) {
                case 0:
                    redDown = opaqueColour & 0x1F;
                    if (redDown >= 0x1FU) {
                        brighter = 0;
                    } else {
                        brighter = redDown + 1;
                    }
                    g_TeamLogoClut[g_TeamLogoPenColor] =
                        brighter |
                        (g_TeamLogoClut[g_TeamLogoPenColor] & 0xFFE0);
                    return;
                case 1:
                    slotValue = (colour >> 5) & 0x1F;
                    if (slotValue < 0x1FU) {
                        brighter = (slotValue + 1) << 5;
                    } else {
                        brighter = 0;
                    }
                    g_TeamLogoClut[g_TeamLogoPenColor] =
                        brighter |
                        (g_TeamLogoClut[g_TeamLogoPenColor] & 0xFC1F);
                    return;
                case 2:
                    blueDown = (colour >> 0xA) & 0x1F;
                    if (blueDown < 0x1FU) {
                        brighter = (blueDown + 1) << 0xA;
                    } else {
                        brighter = 0;
                    }
                    g_TeamLogoClut[g_TeamLogoPenColor] =
                        brighter |
                        (g_TeamLogoClut[g_TeamLogoPenColor] & 0x83FF);
                    return;
                default:
                    return;
                }
            }
        } else {
            if (g_PadPressed & PAD_UP) {
                PlaySoundCue(1);
                prevChannel = 2;
                if (g_TeamLogoColorChannel > 0) {
                    prevChannel = g_TeamLogoColorChannel - 1;
                }
                g_TeamLogoColorChannel = prevChannel;
            }
            if (g_PadPressed & PAD_DOWN) {
                PlaySoundCue(1);
                nextChannel = 0;
                if (g_TeamLogoColorChannel < 2) {
                    nextChannel = g_TeamLogoColorChannel + 1;
                }
                g_TeamLogoColorChannel = nextChannel;
            }
        }
    }
}

/*
 * Drawing on the canvas: the pad moves the pen, one button lays the
 * brush down and the other rubs it out, both at whatever size the brush
 * is set to. Every plot is a nibble inside a canvas word.
 */
void EditLogoCanvas(void) {
    s32 eraseStamp;
    u32 pixelValue;
    s32 plotStamp;
    u16 pressed = g_PadPressed;
    u16 held = g_PadHeld;

    if ((held & PAD_CIRCLE) && (g_TeamLogoPaintArmed != 0)) {
        if (pressed & PAD_CIRCLE) {
            PlaySoundCue(4);
        }
        PaintTeamLogoBrush(GetTeamLogoPenSlot()->low);
    }
    if (held & PAD_SQUARE) {
        if (pressed & PAD_SQUARE) {
            PlaySoundCue(4);
        }
        PaintTeamLogoBrush(0);
    }
    if (pressed & 0x40) {
        PlaySoundCue(2);
        g_TeamLogoPaletteMode = 1;
    }
    if (pressed & 0x10) {
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
    if ((held & 8) && (g_TeamLogoExpertMode != 0)) {
        if (held & 4) {
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
        } else if ((g_TeamLogoDpadRepeatTimer == 0x14) || (g_TeamLogoDpadRepeatTimer == 1)) {
            if (held & 0x1000) {
                ScrollTeamLogoUp();
            }
            if (held & 0x4000) {
                ScrollTeamLogoDown();
            }
            if (held & 0x8000) {
                ScrollTeamLogoLeft();
            }
            if (held & 0x2000) {
                ScrollTeamLogoRight();
            }
        }
    } else {
        eraseStamp = 0;
        if ((g_TeamLogoDpadRepeatTimer == 0x14) || (g_TeamLogoDpadRepeatTimer == 1) || (held & 5)) {
            plotStamp = 0;
            if (held & PAD_UP) {
                if (g_TeamLogoCursorY > 0) {
                    g_TeamLogoCursorY -= 1;
                    plotStamp = 1;
                } else if (g_TeamLogoViewY > 0) {
                    g_TeamLogoViewY -= 1;
                    plotStamp = 1;
                }
            }
            if (held & PAD_DOWN) {
                if ((g_TeamLogoCursorY + g_TeamLogoBrushSize) < 0x20) {
                    g_TeamLogoCursorY += 1;
                    plotStamp = 1;
                } else if (g_TeamLogoViewY < 0x20) {
                    g_TeamLogoViewY += 1;
                    plotStamp = 1;
                }
            }
            if (held & PAD_LEFT) {
                if (g_TeamLogoCursorX > 0) {
                    g_TeamLogoCursorX -= 1;
                    eraseStamp = 1;
                } else if (g_TeamLogoViewX > 0) {
                    g_TeamLogoViewX -= 1;
                    eraseStamp = 1;
                }
            }
            if (held & PAD_RIGHT) {
                if ((g_TeamLogoCursorX + g_TeamLogoBrushSize) < 0x20) {
                    g_TeamLogoCursorX += 1;
                    eraseStamp = 1;
                } else if (g_TeamLogoViewX < 0x20) {
                    g_TeamLogoViewX += 1;
                    eraseStamp = 1;
                }
            }
            if ((held & (PAD_SQUARE | PAD_CIRCLE)) && ((eraseStamp != 0) || (plotStamp != 0))) {
                PlaySoundCue(4);
            }
        }
    }
    if ((pressed & 2) && (g_TeamLogoExpertMode != 0)) {
        PlaySoundCue(4);
        pixelValue = GetTeamLogoCanvasPixel(
            g_TeamLogoViewX + g_TeamLogoCursorX,
            g_TeamLogoViewY + g_TeamLogoCursorY);
        if (pixelValue == 0) {
            pixelValue = g_TeamLogoPenColor;
        }
        g_TeamLogoPenColor = pixelValue;
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
