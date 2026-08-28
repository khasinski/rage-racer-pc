#include "common.h"
#include "game/menu_internal.h"
#include "game/audio.h"
#include "game/menu_types.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_types.h"
#include "game/scratchpad.h"
#include "game/screens.h"
#include "game/state.h"
#include "psyq/gpu.h"

typedef s32 TeamLogoRotationOffset;

typedef union TeamLogoRotationBufferAddress {
    TeamLogoRotationOffset rotationOffset;
    s32 value;
    s32 wordIndex;
    u32 *wordPointer;
} TeamLogoRotationBufferAddress;

typedef union MenuOrderingTableAddress {
    s32 byteOffset;
    s32 value;
    u8 *bytes;
    void *pointer;
} MenuOrderingTableAddress;

static inline TeamLogoColorSlot *GetTeamLogoPenSlot(void) {
    TeamLogoColorAddress address;

    address.index = &g_TeamLogoPenColor;
    return address.slot;
}

/* Identical to the declaration in game/menu.h; this unit cannot include that
 * header yet because four of its own definitions still disagree with it
 * (ScrollTeamLogoUp, GameDrawMenuButton, g_RankingRecords, g_TimeRecords). */
void FlipTeamLogoVertical(void);
void ScrollTeamLogoUp(void);
void ScrollTeamLogoDown(void);
void ScrollTeamLogoLeft(void);
void ScrollTeamLogoRight(void);

/* The logo canvas is 64 rows of eight words, and each word packs eight
 * four-bit pixels. Flipping a row therefore swaps word j with word 7 - j and
 * reverses the nibble order inside both. */
void FlipTeamLogoHorizontal(void) {
    u32 *base;
    s32 row;
    s32 lastWord;
    s32 rowOffset;
    u32 *lowWordPtr;
    s32 colOffset;
    s32 highIndex;
    s32 nibble;
    u32 lowPacked;
    u32 highPacked;
    u32 lowWord;
    u32 highWord;
    u32 shift;

    PlaySoundCue(8);

    base = g_TeamLogoCanvas.words[0];
    row = 0;
    lastWord = 7;
    do {
        rowOffset = row * 32;
        lowWordPtr = base;
        colOffset = 0;
        highIndex = lastWord;
        do {
            s32 addr;
            s32 highOffset;
            TeamLogoCanvasAddress lowAddress;
            TeamLogoCanvasAddress highAddress;

            nibble = 0;
            lowPacked = 0;
            highPacked = 0;
            lowAddress.wordPointer = lowWordPtr;
            addr = rowOffset + lowAddress.value;
            lowAddress.value = addr;
            lowWord = *lowAddress.wordPointer;
            addr = highIndex << 2;
            highAddress.wordPointer = base;
            highOffset = addr + highAddress.value;
            addr = rowOffset + highOffset;
            highAddress.value = addr;
            highWord = *highAddress.wordPointer;
            do {
                lowPacked <<= 4;
                shift = nibble * 4;
                lowPacked |= (lowWord >> shift) & 0xF;
                highPacked <<= 4;
                highPacked |= (highWord >> shift) & 0xF;
                nibble++;
            } while (nibble < 8);
            {
                s32 lowAddr;
                s32 highAddr;
                TeamLogoCanvasAddress lowStoreAddress;
                TeamLogoCanvasAddress highStoreAddress;

                lowStoreAddress.wordPointer = lowWordPtr;
                lowAddr = rowOffset + lowStoreAddress.value;
                lowWordPtr++;
                colOffset += 4;
                highAddr = highIndex << 2;
                highStoreAddress.wordPointer = base;
                highAddr += highStoreAddress.value;
                highOffset = highAddr;
                highAddr = rowOffset + highOffset;
                highStoreAddress.value = highAddr;
                lowStoreAddress.value = lowAddr;
                *highStoreAddress.wordPointer = lowPacked;
                *lowStoreAddress.wordPointer = highPacked;
            }
            highIndex--;
        } while (colOffset < 0x10);
        row++;
    } while (row < 0x40);
}

void RotateTeamLogoCcw(void) {
    s32 i;
    s32 j;
    s32 k;
    s32 limit;
    u32 *base;
    u32 *srcStart;
    u32 *src;
    u32 *stackBase;
    u32 *dst;
    s32 shift;
    u32 value1;
    u32 value2;
    u32 saved[512];
    TeamLogoCanvasAddress sourceAddress;
    TeamLogoRotationBufferAddress copyAddress;
    TeamLogoRotationBufferAddress destinationIndex;
    TeamLogoRotationBufferAddress destinationBase;

    PlaySoundCue(8);

    base = g_TeamLogoCanvas.words[0];
    i = 0;
    limit = 7;
    do {
        j = 0;
        value2 = limit - i;
        value2 <<= 2;
        sourceAddress.wordPointer = base;
        sourceAddress.value = value2 + sourceAddress.value;
        srcStart = sourceAddress.wordPointer;
        do {
            k = 0;
            src = srcStart;
            do {
                destinationIndex.rotationOffset =
                    ((i * 8 + k) * 8 + j) << 2;
                dst = destinationIndex.wordPointer;
                destinationIndex.wordPointer = dst;
                destinationBase.wordPointer = (stackBase = saved);
                destinationIndex.value =
                    destinationIndex.rotationOffset + destinationBase.value;
                dst = destinationIndex.wordPointer;
                shift = (limit - k) << 2;
                *dst = 0;
                value1 = (src[0x38] >> shift) & 0xF;
                value1 <<= 4;
                *dst = value1;
                value2 = (src[0x30] >> shift) & 0xF;
                value2 |= value1;
                value2 <<= 4;
                *dst = value2;
                value1 = (src[0x28] >> shift) & 0xF;
                value1 |= value2;
                value1 <<= 4;
                *dst = value1;
                value2 = (src[0x20] >> shift) & 0xF;
                value2 |= value1;
                value2 <<= 4;
                *dst = value2;
                value1 = (src[0x18] >> shift) & 0xF;
                value1 |= value2;
                value1 <<= 4;
                *dst = value1;
                value2 = (src[0x10] >> shift) & 0xF;
                value2 |= value1;
                value2 <<= 4;
                *dst = value2;
                value1 = (src[0x08] >> shift) & 0xF;
                value1 |= value2;
                value1 <<= 4;
                *dst = value1;
                value2 = (src[0x00] >> shift) & 0xF;
                k++;
                value2 |= value1;
                *dst = value2;
            } while (k < 8);
            j++;
            srcStart += 0x40;
        } while (j < 8);
        i++;
    } while (i < 8);

    i = 0;
    dst = base;
    copyAddress.wordPointer = stackBase;
    value1 = copyAddress.value;
    do {
        copyAddress.value = value1;
        value2 = *copyAddress.wordPointer;
        value1 += 4;
        i++;
        *dst = value2;
        dst++;
    } while (i < 512);
}

void RotateTeamLogoCw(void) {
    s32 i;
    s32 j;
    s32 k;
    u32 *rowBase;
    u32 *base;
    u32 *srcStart;
    u32 *src;
    u32 *stackBase;
    u32 *dst;
    s32 shift;
    u32 value1;
    u32 value2;
    u32 saved[512];
    TeamLogoRotationBufferAddress copyAddress;
    TeamLogoRotationBufferAddress indexAddress;

    PlaySoundCue(8);

    i = 0;
    base = g_TeamLogoCanvas.words[0];
    rowBase = base;
    do {
        j = 0;
        srcStart = rowBase;
        do {
            k = 0;
            src = srcStart;
            do {
                indexAddress.wordIndex = (i * 8 + k) * 8;
                dst = indexAddress.wordPointer;
                indexAddress.wordPointer = dst;
                indexAddress.wordIndex += 7;
                dst = indexAddress.wordPointer;
                indexAddress.wordPointer = dst;
                indexAddress.wordIndex -= j;
                dst = indexAddress.wordPointer;
                indexAddress.wordPointer = dst;
                indexAddress.wordIndex *= 4;
                copyAddress.wordPointer = (stackBase = saved);
                indexAddress.value += copyAddress.value;
                dst = indexAddress.wordPointer;
                *dst = 0;
                value1 = src[0x00];
                shift = k * 4;
                value1 = (value1 >> shift) & 0xF;
                value1 <<= 4;
                *dst = value1;
                value2 = (src[0x08] >> shift) & 0xF;
                value2 |= value1;
                value2 <<= 4;
                *dst = value2;
                value1 = (src[0x10] >> shift) & 0xF;
                value1 |= value2;
                value1 <<= 4;
                *dst = value1;
                value2 = (src[0x18] >> shift) & 0xF;
                value2 |= value1;
                value2 <<= 4;
                *dst = value2;
                value1 = (src[0x20] >> shift) & 0xF;
                value1 |= value2;
                value1 <<= 4;
                *dst = value1;
                value2 = (src[0x28] >> shift) & 0xF;
                value2 |= value1;
                value2 <<= 4;
                *dst = value2;
                value1 = (src[0x30] >> shift) & 0xF;
                value1 |= value2;
                value1 <<= 4;
                *dst = value1;
                value2 = src[0x38];
                k++;
                value2 = (value2 >> shift) & 0xF;
                value2 |= value1;
                *dst = value2;
            } while (k < 8);
            j++;
            srcStart += 0x40;
        } while (j < 8);
        i++;
        rowBase++;
    } while (i < 8);

    i = 0;
    dst = base;
    copyAddress.wordPointer = stackBase;
    value1 = copyAddress.value;
    do {
        copyAddress.value = value1;
        value2 = *copyAddress.wordPointer;
        value1 += 4;
        i++;
        *dst = value2;
        dst++;
    } while (i < 512);
}


void UpdateTeamLogoCanvas(void) {
    s32 cursorX;
    s32 redUp;
    s32 greenUp;
    s32 blueUp;
    s32 wordColumn;
    s32 nibbleShift;
    s32 nextTool;
    s32 darker;
    s32 brighter;
    s32 plotStamp;
    s32 eraseStamp;
    s32 plotColumn;
    s32 eraseColumn;
    s32 plotRow;
    s32 eraseRow;
    s32 prevChannel;
    s32 prevSlot;
    s32 nextSlot;
    s32 rowWords;
    s32 nextChannel;
    u16 *clutEntry;
    u16 opaqueColour;
    u16 colour;
    u32 redDown;
    u32 slotValue;
    u32 blueDown;
    u32 pixelValue;
    TeamLogoPixelWord *canvasWord;
    s32 pixelX;

    {
        s32 input;
        s32 held;
        s32 repeat;
        volatile u16 *state = &g_PadHeld;

        input = (*state & 5) ? 0 : 3;
        if (*state & g_TeamLogoDpadRepeatMask) {
            held = g_TeamLogoDpadRepeatTimer;
            repeat = 0x14;
            if (held < input + repeat) {
                repeat = held + 1;
            }
            g_TeamLogoDpadRepeatTimer = repeat;
        } else {
            g_TeamLogoDpadRepeatTimer = 0;
        }
    }
    {
        u16 held = g_PadHeld;

        g_TeamLogoDpadRepeatMask = held & 0xF000;
        if (!(held & 0x20)) {
            g_TeamLogoPaintArmed = 1;
        }
    }
    if (g_TeamLogoExpertMode != 0) {
        if (g_PadPressed & PAD_SELECT) {
            g_TeamLogoGuideModePrev = g_TeamLogoGuideMode;
            nextTool = 0;
            if (g_TeamLogoGuideMode < 2) {
                nextTool = g_TeamLogoGuideMode + 1;
            }
            g_TeamLogoGuideMode = nextTool;
        }
    } else {
        g_TeamLogoGuideMode = 1;
    }
    if (g_TeamLogoPaletteMode == 1) {
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
    } else {
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
}


void RestoreTeamLogoClut(void) { LoadImage(&g_TeamLogoClutRect, &g_TeamLogoBlankClut); }

void UploadTeamLogoClut(void) { LoadImage(&g_TeamLogoClutRect, g_TeamLogoClut); }


void DrawMenuLightBurst(s32 arg) {
    void *s3;
    MenuLightBurstBand l1;
    MenuLightBurstBand l2;
    s3 = &SCRATCH_OT_BASE_AS(OT_TYPE)[0x2BF];
    l1 = g_MenuLightBurstBandX;
    l2 = g_MenuLightBurstBandY;

    if (arg == 0) {
        g_MenuLightBurstLevel = 0;
        return;
    }
    if (arg < 0) {
        g_MenuLightBurstLevel += arg;
        if (g_MenuLightBurstLevel < 0) {
            g_MenuLightBurstLevel = 0;
        }
    }
    if (g_MenuLightBurstLevel > 0) {
        s32 s0;
        s32 s1;
        s32 s2;
        u8 *prim;
        s32 value;
        u32 scaled;

        SetDrawClipRect(s3, 0, 0, 0x140, 0x1E0);

        s0 = 0;
        s2 = 0;
        s1 = 0x00300000;
        do {
            u32 cnt = g_MenuLightBurstLevel;
            u8 c1;
            value = cnt * 11;
            scaled = value / 256U;
            c1 = (cnt * 75) / 256;
            value = (u8)scaled;
            DrawGradientLine(s3, s1 >> 16, 0xAA, s2 >> 16, 0x1E0, value, value, value, c1, c1, c1, 0x60);
            s2 += 0x000A0000;
            s1 += 0x00070000;
            s0++;
        } while (s0 < 0x21);

        s0 = 0;
        do {
            s32 x0 = l1.values[s0];
            s32 y0 = l2.values[s0];
            s16 x1 = (0xA0 - (u16)l1.values[s0]) * 2;
            s32 v = ((((u16)l2.values[s0] - 0xAA) << 7) / 309 + 0x16) * g_MenuLightBurstLevel;
            value = v;
            scaled = value / 512U;
            value = (u8)scaled;
            DrawSolidRect(s3, x0, y0, x1, 2, value, value, value, 0x60);
            s0++;
        } while (s0 < 0x21);

        prim = SCRATCH_PRIM_CURSOR_AS(u8);
        SetPolyG4(prim);
        SetSemiTrans(prim, 0);
        {
            RenderBufferAddress primAddress;
            POLY_G4 *quad;
            s32 x = g_MenuLightBurstLevel;

            primAddress.bytes = prim;
            quad = primAddress.polyG4;
            value = x / 5 + (x >> 31);
            scaled = value - (x >> 31);
            quad->x3 = 0x13F;
            quad->x1 = 0x13F;
            quad->y1 = 0x28;
            quad->y0 = 0x28;
            quad->x2 = 0;
            quad->x0 = 0;
            quad->y3 = 0x1DF;
            quad->y2 = 0x1DF;
            quad->r1 = 0;
            quad->r0 = 0;
            quad->g1 = 0;
            quad->g0 = 0;
            quad->b1 = 0;
            quad->b0 = 0;
            quad->r3 = scaled;
            quad->r2 = scaled;
            quad->g3 = scaled;
            quad->g2 = scaled;
            quad->b3 = scaled;
            quad->b2 = scaled;
        }
        {
            u8 *oldPrim = prim;
            prim += sizeof(POLY_G4);
            AddPrim(s3, oldPrim);
        }
        SCRATCH_PRIM_CURSOR_AS(u8) = prim;
        SetDrawClipRect(s3, 0x48, 0, 0x140, 0x1E0);
    }
    if (arg > 0) {
        g_MenuLightBurstLevel += arg;
        if (g_MenuLightBurstLevel >= 0x201) {
            g_MenuLightBurstLevel = 0x200;
        }
    }
}

typedef union PackedCoordinate {
    s32 value;
    struct {
        u16 fraction;
        u16 integer;
    } parts;
} PackedCoordinate;

/* GCC 2.6.3 needs the concrete outer bound for the indexed view below. */

/* The animated five-row ranking/time-record panel. */
s32 DrawRankingTable(s32 *progress, s32 step, s32 ranking) {
    char text[16];
    OT_TYPE *ot;
    s32 phase;
    u32 slide;
    s32 headerTextureU;
    s16 panelY;
    s16 contentY;
    s16 rowY;
    s32 row;
    s16 rowYStep;
    s32 loopClut;
    s32 spriteHeight;
    s32 spriteOne;
    s32 badgeMask;
    PackedCoordinate badgeX;
    s16 car;
    s32 badgeXWord;
    s16 rectLeft;

    ot = SCRATCH_OT_BASE_AS(OT_TYPE);
    if (step == 0) {
        *progress = 0;
        /* Initialization-only call; its caller ignores the return value. */
        return 0;
    }

    if (step < 0) {
        s32 currentValue;
        s32 value;

        currentValue = *progress;
        value = step + currentValue;
        *progress = value;
        if (value < 0) {
            *progress = 0;
        }
    }

    phase = *progress;
    if (phase >= 0) {
        u32 slidePhase;

        if (phase >= 13) {
            phase = 12;
        }
        slidePhase = phase;
        slide = (slidePhase * -1120) >> 5;
        panelY = slide + 0x21A;

        if (g_CourseIndex >= 4) {
            DrawSprite(ot, 0xA4, (s16)(slide + 0x1EA), 0x30, 0x18,
                             0xCC, 0x38, 0, 0, 0, 0x20F, 1, 0, 0x3C);
        }
        DrawSprite(ot, 0xC8, (s16)(slide + 0x218), 0x20, 0x28,
                         0x48, 0xD8, 0, 0, 0, 0x220, 1, 0, 0x19);

        {
            s32 headerClut;
            s32 headerFlags;
            s32 contentYWide;

            headerClut = 0x244;
            headerFlags = 0x3B;
            headerTextureU = 0xA4;
            contentYWide = slide + 0x26C;
            contentY = contentYWide;
            DrawSprite(ot, 0xA4, contentY, 0x48, 0x10, 0x48, 0xAC,
                             0, 0, 0, headerClut, 1, 1, headerFlags);
            switch (SeriesCourseIndex()) {
            case 0:
                DrawSprite(ot, 0xA4, (s16)(slide + 0x25C), 0x54, 0x10,
                                 0, 0x9C, 0, 0, 0, headerClut, 1, 1, headerFlags);
                DrawSprite(ot, 0xEC, contentY, 0x20, 0x10, 0x44, 0xB4,
                                 0, 0, 0, headerClut, 1, 1, 0x3A);
                break;
            case 1:
                DrawSprite(ot, 0xA4, (s16)(slide + 0x25C), 0x4C, 0x10,
                                 0x54, 0x9C, 0, 0, 0, headerClut, 1, 1,
                                 headerFlags);
                DrawSprite(ot, 0xEC, contentY, 0x20, 0x10, 0x64, 0xB4,
                                 0, 0, 0, headerClut, 1, 1, 0x3A);
                break;
            case 2:
                DrawSprite(ot, 0xA4, (s16)(slide + 0x25C), 0x48, 0x10,
                                 0, 0xAC, 0, 0, 0, headerClut, 1, 1,
                                 headerFlags);
                DrawSprite(ot, 0xEC, contentY, 0x20, 0x10, 0x84, 0xB4,
                                 0, 0, 0, headerClut, 1, 1, 0x3A);
                break;
            case 3: {
                DrawSprite(ot, 0xA4, (s16)(slide + 0x25C), 0x5C, 0x10,
                                 headerTextureU, 0x9C, 0, 0, 0,
                                 headerClut, 1, 1,
                                 headerFlags);
                DrawSprite(ot, 0xEC, contentY, 0x20, 0x10,
                                 headerTextureU, 0xB4, 0, 0, 0,
                                 headerClut, 1, 1, 0x3A);
                break;
            }
            }
        }

        if (ranking != 0) {
            rowY = panelY + 0xA;
            DrawSprite(ot, 0x18, rowY, 0x12, 0x10, 0, 0x7C,
                             0, 0, 0, 0x244, 1, 1, 0x3B);
            DrawSprite(ot, 0x2B, rowY, 0x14, 0x10, 0xC0, 0x8C,
                             0, 0, 0, 0x244, 1, 1, 0x3B);
            DrawSprite(ot, 0x41, rowY, 0x2C, 0x10, 0xD4, 0x8C,
                             0, 0, 0, 0x244, 1, 1, 0x3B);
        } else {
            rowY = panelY + 0xA;
            DrawSprite(ot, 0x18, rowY, 0x1C, 0x10, 0x44, 0x7C,
                             0, 0, 0, 0x244, 1, 1, 0x3B);
            DrawSprite(ot, 0x34, rowY, 0x14, 0x10, 0xC0, 0x8C,
                             0, 0, 0, 0x244, 1, 1, 0x3B);
            DrawSprite(ot, 0x4A, rowY, 0x2C, 0x10, 0xD4, 0x8C,
                             0, 0, 0, 0x244, 1, 1, 0x3B);
        }
        GameDrawMenuButton(0, panelY, 0x99, 0x23, 0, 0, 0, 0, 0, 0, 0);

        rectLeft = 0;
        loopClut = 0x244;
        spriteHeight = 0x10;
        spriteOne = 1;
        badgeX.value = 0;
        /* Keeps GCC 2.6.3's packed-coordinate allocation deterministic. */
        
        badgeMask = -65536;
        badgeX.value &= badgeMask;
        row = 0;
        badgeX.parts.integer = 0xE8;
        rowYStep = 0x82;
        do {
            if (ranking != 0) {
                FormatLapTime(
                    text,
                    g_RankingRecords[CourseSeries(g_CourseIndex)]
                                    [(SeriesCourseIndex())][row].raceTime);
                rowY = panelY + rowYStep;
                DrawLargeText(0x36, rowY, text, 0x7F, 0x7F, 0x7F,
                                  loopClut, 0x20);
                DrawLargeText(
                    0x77, rowY,
                    g_RankingRecords[CourseSeries(g_CourseIndex)]
                                    [(SeriesCourseIndex())][row].driverName,
                    0x7F, 0x7F, 0x7F, loopClut, 0xA0);
            } else {
                FormatLapTime(
                    text,
                    g_TimeRecords[CourseSeries(g_CourseIndex)]
                                 [(SeriesCourseIndex())][row].raceTime);
                rowY = panelY + rowYStep;
                DrawLargeText(0x36, rowY, text, 0x7F, 0x7F, 0x7F,
                                  loopClut, 0x20);
                DrawLargeText(
                    0x77, rowY,
                    g_TimeRecords[CourseSeries(g_CourseIndex)]
                                 [(SeriesCourseIndex())][row].driverName,
                    0x7F, 0x7F, 0x7F, loopClut, 0xA0);
            }

            DrawSprite(ot, 0x17, (s16)(panelY + rowYStep), 8,
                             spriteHeight, (s16)(row * 8 + 8), 0x18,
                             0, 0, 0, loopClut, spriteOne, spriteOne, 0x3B);
            DrawSprite(ot, 0xA7, (s16)(panelY + rowYStep), 8,
                             spriteHeight, 0x58, 0x28, 0, 0, 0, loopClut,
                             spriteOne, spriteOne, 0x3B);
            DrawSprite(ot, 0xDF, (s16)(panelY + rowYStep), 8,
                             spriteHeight, 0x58, 0x28, 0, 0, 0, loopClut,
                             spriteOne, spriteOne, 0x3B);

            if (ranking != 0) {
                car = (u16)g_RankingRecords[CourseSeries(g_CourseIndex)]
                                             [(SeriesCourseIndex())][row].carIndex;
            } else {
                car = (u16)g_TimeRecords[CourseSeries(g_CourseIndex)]
                                          [(SeriesCourseIndex())][row].carIndex;
            }
            switch (car) {
                case 0:
                case 1:
                case 2:
                case 10:
                    DrawSprite(ot, 0xAE, (s16)(panelY + rowYStep),
                                     0x14, spriteHeight, 0x50,
                                     0xBC, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3B);
                    break;
                case 3:
                    DrawSprite(ot, 0xAF, (s16)(panelY + rowYStep),
                                     0x20, spriteHeight, 0,
                                     0xBC, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3B);
                    break;
                case 4:
                case 5:
                case 6:
                case 11:
                    DrawSprite(ot, 0xAF, (s16)(panelY + rowYStep),
                                     0x20, spriteHeight, 0x64,
                                     0xBC, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3B);
                    break;
                case 7:
                case 8:
                case 9:
                case 12:
                    DrawSprite(ot, 0xAF, (s16)(panelY + rowYStep),
                                     0x30, spriteHeight, 0x22,
                                     0xBC, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3B);
                    break;
            }

            switch (car) {
                case 0:
                    badgeXWord = badgeX.value;
                    DrawSprite(ot, badgeXWord >> 16,
                                     (s16)(panelY + rowYStep),
                                     0x2A, spriteHeight, 0x16,
                                     0x30, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 1:
                    badgeXWord = badgeX.value;
                    DrawSprite(ot, badgeXWord >> 16,
                                     (s16)(panelY + rowYStep),
                                     0x20, spriteHeight, 0x48,
                                     0x30, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 2:
                    DrawSprite(ot, 0xE9, (s16)(panelY + rowYStep),
                                     0x20, spriteHeight, 0x7C,
                                     0x30, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 10:
                    DrawSprite(ot, 0xE9, (s16)(panelY + rowYStep),
                                     0x2C, spriteHeight, 0xA4,
                                     0x30, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 3:
                    DrawSprite(ot, 0xE7, (s16)(panelY + rowYStep),
                                     0x34, spriteHeight, 0,
                                     0x40, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 4:
                    DrawSprite(ot, 0xE9, (s16)(panelY + rowYStep),
                                     0x28, spriteHeight, 0x74,
                                     0x50, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 5:
                    DrawSprite(ot, badgeX.value >> 16,
                                     (s16)(panelY + rowYStep),
                                     0x2A, spriteHeight, 0x3E,
                                     0x50, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 6:
                    badgeXWord = badgeX.value;
                    DrawSprite(ot, badgeXWord >> 16,
                                     (s16)(panelY + rowYStep),
                                     0x20, spriteHeight, 0xB0,
                                     0x50, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 11:
                    badgeXWord = badgeX.value;
                    DrawSprite(ot, badgeXWord >> 16,
                                     (s16)(panelY + rowYStep),
                                     0x2A, spriteHeight, 0x0A,
                                     0x60, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 7:
                    badgeXWord = badgeX.value;
                    DrawSprite(ot, badgeXWord >> 16,
                                     (s16)(panelY + rowYStep),
                                     0x28, spriteHeight, 0x40,
                                     0x40, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 8:
                    DrawSprite(ot, 0xE9, (s16)(panelY + rowYStep),
                                     0x22, spriteHeight, 0x7A,
                                     0x40, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 9:
                    DrawSprite(ot, 0xE9, (s16)(panelY + rowYStep),
                                     0x30, spriteHeight, 0xA0,
                                     0x40, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 12:
                    badgeXWord = badgeX.value;
                    DrawSprite(ot, badgeXWord >> 16,
                                     (s16)(panelY + rowYStep),
                                     0x30, spriteHeight, 0x04,
                                     0x50, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
            }

            rowYStep += 0x20;
            row++;
        } while (row < 5);

        {
            s32 suffixX;
            s32 textShade;
            s32 textClut;
            s32 textFlags;
            const char *lastSuffix;

            suffixX = 0x1E;
            
            textShade = 0x7F;
            textClut = 0x244;
            textFlags = 0x20;
            DrawLargeText(suffixX, (s16)(panelY + 0x82),
                              g_MsgOrdinalSt, textShade, textShade,
                              textShade, textClut, textFlags);
            DrawLargeText(suffixX, (s16)(panelY + 0xA2),
                              g_MsgOrdinalNd, textShade, textShade,
                              textShade, textClut, textFlags);
            DrawLargeText(0x1F, (s16)(panelY + 0xC2),
                              g_MsgOrdinalRd, textShade, textShade,
                              textShade, textClut, textFlags);
            lastSuffix = g_MsgOrdinalTh;
            DrawLargeText(suffixX, (s16)(panelY + 0xE2), lastSuffix,
                              textShade, textShade, textShade, textClut,
                              textFlags);
            DrawLargeText(suffixX, (s16)(panelY + 0x102), lastSuffix,
                              textShade, textShade, textShade, textClut,
                              textFlags);
        }

        {
            u32 *rectOt;
            u32 *rectCallOt;
            s32 rectX;
            s16 rectY;
            s32 rectHeight;
            s32 rectAlpha;

            rectOt = (u32 *)(ot + 1);
            rectCallOt = rectOt;
            rectX = rectLeft;
            rectY = panelY + 0x7A;
            rectHeight = 0xA0;
            rectAlpha = 0xFF;
            DrawRectOutline(rectCallOt, rectX, rectY, 0x124,
                                rectHeight, 0xB4, 0xB4, 0xB4, rectAlpha);
            rectCallOt = rectOt;
            DrawSolidRect(rectCallOt, rectX, rectY, 0x124, rectHeight,
                              0, 0, 0, rectAlpha);
        }
    }

    if (step >= 0) {
        s32 nextProgress;

        nextProgress = step + *progress;
        if (nextProgress >= 15) {
            *progress = 15;
            return 1;
        }
        *progress = nextProgress;
    }
    return 0;
}
