#include "game/diagnostics.h"
#include "game/state.h"
#include "game/input_internal.h"
#include "game/render.h"
#include <stdlib.h>
#include <stdio.h>

/* Attach the two retail 0x28-byte pad buffers to the host input backend. */
void GameInitPad(void) {
    InitPad(g_PadBuffers, 0x28, g_PadBuffers + 0x28, 0x28);
}

/* The live mapping UpdatePadState reads: the pad's eight masks at +0,
 * the NeGcon's eight at +0x10. */

/*
 * Installs the two selected presets into the live mapping table. Both rows are
 * copied in the same 8-iteration loop, hence the pair of source and
 * destination cursors.
 */
void LoadPadButtonMapping(s32 mapping0, s32 mapping1) {
    s32 i;
    u16 *dst0;
    u16 *dst1;
    u16 *src0;
    u16 *src1;
    u16 *table;

    i = 0;
    dst0 = g_PadButtonMapping;
    dst1 = dst0 + 8;
    table = g_NegconButtonPresets;
    src1 = table + mapping1 * 8;
    table = g_PadButtonPresets;
    src0 = table + mapping0 * 8;

    do {
        u16 mask;

        *dst0 = *src0++;
        i++;
        mask = *src1++;
        dst0++;
        *dst1++ = mask;
    } while (i < 8);
}


/* Re-applies the button mapping from the two saved selections. */
void ApplyPadButtonMapping(void) {
    LoadPadButtonMapping(g_PadMappingIndex, g_NegconMappingIndex);
}


void UpdatePadState(void) {
    s32 mask;
    s32 v;
    s32 t;
    s32 n;
    s32 c;
    s16 c1;
    s16 c2;
    s16 d;
    s16 neutral;
    s16 r;
    u8 *raw;
    PadState *pad;

    raw = g_PadBuffers;
    pad = &g_PadState;

    pad->status = raw[0];
    g_PadType = g_PadBufferType;
    if (raw[0] != 0) {
        v = PAD_ERROR_STATE_DISCONNECTED;
        g_PadErrorState = v;
        g_PadValidateCountdown = 0x22;
        g_PadErrorHoldBits |= 0x10;
    } else {
        if (g_PadValidateCountdown != 0) {
            g_PadValidateCountdown--;
            if (g_PadBufferType == 0x23) {
                mask = ~(g_PadBufferButtonsLow | (g_PadBufferButtonsHigh << 8));
                if (!(((mask & 0x5000) != 0x5000) && ((mask & 0xA000) != 0xA000) &&
                    ((mask & 0x1C4) == 0))) {
                    v = PAD_ERROR_STATE_INVALID_INPUT;
        g_PadErrorState = v;
        g_PadValidateCountdown = 0x22;
        g_PadErrorHoldBits |= 0x10;
                }
            }
        }
    }
    g_PadErrorHoldBits = g_PadErrorHoldBits >> 1;
    if (g_PadErrorHoldBits != 0) {
        raw[1] = 0;
        pad->type = 0;
    } else {
        g_PadErrorState = PAD_ERROR_STATE_NONE;
    }
    if (raw[1] == 0x41) {
        pad->prevHeld = pad->held;
        pad->held = ~((raw[2] << 8) | raw[3]);
        pad->pressed = pad->held & ~pad->prevHeld;
        t = (pad->held >> 13) & 1;
        n = t;
        c = g_NegconSteerRange[g_NegconMaxTwist];
        pad->twist = ((pad->held & PAD_LEFT) ? ((n - 1) * c) : (n * c)) + 0x80;
        pad->buttonI = (pad->held & PAD_CROSS) ? 0x6A : 0;
        pad->buttonII = (pad->held & PAD_SQUARE) ? 0x6A : 0;
        pad->buttonL = (pad->held & PAD_L1) ? 0x6A : 0;
    } else if (raw[1] == 0x23) {
        pad->prevHeld = pad->held;
        pad->held = ~((raw[2] << 8) | raw[3]);
        pad->twist = raw[4];;
        pad->buttonI = raw[5] - g_NegconNeutralI;
        pad->buttonII = raw[6] - g_NegconNeutralII;
        pad->buttonL = raw[7] - g_NegconNeutralL;
        if (pad->buttonI < 0) {
            pad->buttonI = 0;
        }
        if (pad->buttonII < 0) {
            pad->buttonII = 0;
        }
        if (pad->buttonL < 0) {
            pad->buttonL = 0;
        }
        if (pad->twist >= 0xA3) {
            pad->held |= PAD_RIGHT;
        }
        if (pad->twist < 0x5E) {
            pad->held |= PAD_LEFT;
        }
        if (pad->buttonI >= 0x36) {
            pad->held |= PAD_CROSS;
        }
        if (pad->buttonII >= 0x36) {
            pad->held |= PAD_SQUARE;
        }
        if (pad->buttonL >= 0x36) {
            pad->held |= PAD_L1;
        }
        pad->pressed = pad->held & ~pad->prevHeld;
    } else {
        if (g_PadErrorState == PAD_ERROR_STATE_NONE) {
            g_PadErrorState = PAD_ERROR_STATE_INVALID_INPUT;
        }
        pad->status = 1;
        pad->held = 0;
        pad->pressed = 0;
        pad->twist = 0x80;
        pad->buttonI = 0;
        pad->buttonII = 0;
        pad->buttonL = 0;
    }
    pad->pressedRepeat = pad->held & ~g_PadPrevHeld;
    if (pad->held != 0 && pad->held == g_PadPrevHeld) {
        if (g_PadRepeatTimer[0] == 0x1E) {
            pad->pressedRepeat = pad->pressedRepeat | pad->held;
        }
        g_PadRepeatTimer[0] = (g_PadRepeatTimer[0] < 0x24) ? (g_PadRepeatTimer[0] + 1) : 0x1E;
    } else {
        g_PadRepeatTimer[0] = 0;
    }
    g_PadPrevHeld = pad->held;
    neutral = g_NegconSteerNeutral + 0x80;
    d = pad->twist - neutral;
    if (d > 0) {
        r = d - g_NegconSteerDeadZone[g_NegconSteerPlay][0];
        if (r < 0) {
            r = 0;
        }
        c1 = g_NegconSteerRange[g_NegconMaxTwist];
        if (r > c1) {
            r = c1;
        }
    } else {
        r = d + g_NegconSteerDeadZone[g_NegconSteerPlay][0];
        if (r > 0) {
            r = 0;
        }
        c2 = g_NegconSteerRange[g_NegconMaxTwist];
        if (r < -c2) {
            r = -c2;
        }
    }
    pad->steer = r;
    if (pad->buttonI >= 0x6B) {
        pad->buttonI = 0x6A;
    }
    if (pad->buttonII >= 0x6B) {
        pad->buttonII = 0x6A;
    }
    if (pad->buttonL >= 0x6B) {
        pad->buttonL = 0x6A;
    }
    g_PadHeld = pad->held;
    g_PadPressed = pad->pressed;
    g_PadPressedRepeat = pad->pressedRepeat;
    g_NegconAnalogI = pad->buttonI;
    g_NegconAnalogII = pad->buttonII;
    g_NegconAnalogL = pad->buttonL;
    g_NegconSteer = pad->steer;
    if (DiagnosticsEnabled("input.debug") &&
        (pad->held != 0 || pad->pressed != 0)) {
        printf("pad input: raw=%02x,%02x,%02x,%02x type=%02x held=%04x pressed=%04x\n",
               raw[0], raw[1], raw[2], raw[3], g_PadType,
               (u16)pad->held, (u16)pad->pressed);
    }
}
