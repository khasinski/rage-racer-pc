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
    TeamLogoPixelWord *canvasWord;
    s32 cursorX;
    s32 eraseColumn;
    s32 eraseRow;
    s32 eraseStamp;
    s32 nibbleShift;
    u32 pixelValue;
    s32 pixelX;
    s32 plotColumn;
    s32 plotRow;
    s32 plotStamp;
    s32 rowWords;
    s32 wordColumn;

    if ((g_PadHeld & PAD_CIRCLE) && (g_TeamLogoPaintArmed != 0)) {
        if (g_PadPressed & PAD_CIRCLE) {
            PlaySoundCue(4);
        }
        for (plotRow = 0; plotRow < g_TeamLogoBrushSize; plotRow++) {
                for (plotColumn = 0; plotColumn < g_TeamLogoBrushSize; plotColumn++) {
                        u16 *p;
                        s32 sum;
                        s32 adj;
                        s32 row;
                        s32 q;
                        s32 rem;

                        p = g_TeamLogoCanvas.halfwords;
                        sum = g_TeamLogoViewX + g_TeamLogoCursorX + plotColumn;
                        adj = sum;
                        row = (g_TeamLogoViewY + g_TeamLogoCursorY + plotRow) * 0x10;
                        if (sum < 0) {
                            adj = sum + 3;
                        }
                        q = adj >> 2;
                        p += row + q;
                        rem = sum - (q * 4);
                        switch (rem) {
                        case 0:
                            *p = (*p & 0xFFF0) |
                                 GetTeamLogoPenSlot()->low;
                            break;
                        case 1:
                            *p = (*p & 0xFF0F) |
                                 (GetTeamLogoPenSlot()->low << 4);
                            break;
                        case 2:
                            *p = (*p & 0xF0FF) |
                                 (GetTeamLogoPenSlot()->low << 8);
                            break;
                        case 3:
                            *p = (*p & 0xFFF) |
                                 (GetTeamLogoPenSlot()->low << 0xC);
                            break;
                        }
                }
        }
    }
    if (g_PadHeld & PAD_SQUARE) {
        if (g_PadPressed & PAD_SQUARE) {
            PlaySoundCue(4);
        }
        for (eraseRow = 0; eraseRow < g_TeamLogoBrushSize; eraseRow++) {
                for (eraseColumn = 0; eraseColumn < g_TeamLogoBrushSize; eraseColumn++) {
                        u16 *p;
                        s32 sum;
                        s32 adj;
                        s32 row;
                        s32 q;
                        s32 rem;

                        sum = g_TeamLogoViewX + g_TeamLogoCursorX + eraseColumn;
                        p = g_TeamLogoCanvas.halfwords;
                        adj = sum;
                        row = g_TeamLogoViewY + g_TeamLogoCursorY + eraseRow;
                        row *= 0x10;
                        if (sum < 0) {
                            adj = sum + 3;
                        }
                        q = adj >> 2;
                        adj = row;
                        p += adj + q;
                        rem = sum - (q * 4);
                        switch (rem) {
                        case 0:
                            *p &= 0xFFF0;
                            break;
                        case 1:
                            *p &= 0xFF0F;
                            break;
                        case 2:
                            *p &= 0xF0FF;
                            break;
                        case 3:
                            *p &= 0xFFF;
                            break;
                        }
                }
        }
    }
    {
        u16 *input = &g_PadPressed;

    if (*input & 0x40) {
        PlaySoundCue(2);
        g_TeamLogoPaletteMode = 1;
    }
    if (*input & 0x10) {
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
    }
    {
        volatile u16 *held = &g_PadHeld;
        u16 heldValue = *held;

    if ((heldValue & 8) && (g_TeamLogoExpertMode != 0)) {
        if (heldValue & 4) {
            if (g_PadPressed & PAD_UP) {
                RotateTeamLogoCw();
            }
            if (g_PadPressed & PAD_DOWN) {
                FlipTeamLogoVertical();
            }
            if (g_PadPressed & PAD_LEFT) {
                RotateTeamLogoCcw();
            }
            if (g_PadPressed & PAD_RIGHT) {
                FlipTeamLogoHorizontal();
            }
        } else if ((g_TeamLogoDpadRepeatTimer == 0x14) || (g_TeamLogoDpadRepeatTimer == 1)) {
            if (heldValue & 0x1000) {
                ScrollTeamLogoUp();
            }
            if (*held & 0x4000) {
                ScrollTeamLogoDown();
            }
            if (*held & 0x8000) {
                ScrollTeamLogoLeft();
            }
            if (*held & 0x2000) {
                ScrollTeamLogoRight();
            }
        }
    } else {
        eraseStamp = 0;
        if ((g_TeamLogoDpadRepeatTimer == 0x14) || (g_TeamLogoDpadRepeatTimer == 1) || (g_PadHeld & 5)) {
            plotStamp = 0;
            if (g_PadHeld & PAD_UP) {
                if (g_TeamLogoCursorY > 0) {
                    g_TeamLogoCursorY -= 1;
                    plotStamp = 1;
                } else if (g_TeamLogoViewY > 0) {
                    g_TeamLogoViewY -= 1;
                    plotStamp = 1;
                }
            }
            if (g_PadHeld & PAD_DOWN) {
                if ((g_TeamLogoCursorY + g_TeamLogoBrushSize) < 0x20) {
                    g_TeamLogoCursorY += 1;
                    plotStamp = 1;
                } else if (g_TeamLogoViewY < 0x20) {
                    g_TeamLogoViewY += 1;
                    plotStamp = 1;
                }
            }
            if (g_PadHeld & PAD_LEFT) {
                if (g_TeamLogoCursorX > 0) {
                    g_TeamLogoCursorX -= 1;
                    eraseStamp = 1;
                } else if (g_TeamLogoViewX > 0) {
                    g_TeamLogoViewX -= 1;
                    eraseStamp = 1;
                }
            }
            if (g_PadHeld & PAD_RIGHT) {
                if ((g_TeamLogoCursorX + g_TeamLogoBrushSize) < 0x20) {
                    g_TeamLogoCursorX += 1;
                    eraseStamp = 1;
                } else if (g_TeamLogoViewX < 0x20) {
                    g_TeamLogoViewX += 1;
                    eraseStamp = 1;
                }
            }
            if ((g_PadHeld & (PAD_SQUARE | PAD_CIRCLE)) && ((eraseStamp != 0) || (plotStamp != 0))) {
                PlaySoundCue(4);
            }
        }
    }
    }
    if ((g_PadPressed & 2) && (g_TeamLogoExpertMode != 0)) {
        PlaySoundCue(4);
        canvasWord = g_TeamLogoCanvas.pixels;
        cursorX = g_TeamLogoViewX + g_TeamLogoCursorX;
        pixelX = cursorX;
        rowWords = (g_TeamLogoViewY + g_TeamLogoCursorY) * 0x10;
        if (cursorX < 0) {
            pixelX = cursorX + 3;
        }
        wordColumn = pixelX >> 2;
        canvasWord += rowWords + wordColumn;
        nibbleShift = cursorX;
        nibbleShift = nibbleShift - (wordColumn * 4);
        switch (nibbleShift) {
        case 0:
            pixelValue = canvasWord[0].value & 0xF;
            break;
        case 1:
            pixelValue = canvasWord[0].bytes[0] / 16;
            break;
        case 2:
            pixelValue = canvasWord[0].bytes[1] & 0xF;
            break;
        case 3:
            pixelValue = canvasWord[0].value >> 0xC;
            break;
        default:
            return;
        }
        if (pixelValue == 0) {
            pixelValue = g_TeamLogoPenColor;
        }
        g_TeamLogoPenColor = pixelValue;
    }
}
