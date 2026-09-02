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

    mapping0 = ClampControllerMappingIndex(mapping0);
    mapping1 = ClampControllerMappingIndex(mapping1);
    for (i = 0; i < 8; i++) {
        g_PadButtonMapping[i] = g_PadButtonPresets[mapping0 * 8 + i];
        g_PadButtonMapping[8 + i] = g_NegconButtonPresets[mapping1 * 8 + i];
    }
}


/* Re-applies the button mapping from the two saved selections. */
void ApplyPadButtonMapping(void) {
    LoadPadButtonMapping(g_PadMappingIndex, g_NegconMappingIndex);
}

static s16 ClampNegconPressure(s32 pressure) {
    if (pressure < 0) return 0;
    if (pressure > 0x6A) return 0x6A;
    return (s16)pressure;
}

static s16 CalibrateNegconSteering(s32 twist) {
    s32 neutral = g_NegconSteerNeutral + 0x80;
    s32 delta = twist - neutral;
    s32 deadZone = g_NegconSteerDeadZone[g_NegconSteerPlay][0];
    s32 range = GetNegconSteerRange();
    s32 steering;

    if (delta > 0) {
        steering = delta - deadZone;
        if (steering < 0) steering = 0;
        if (steering > range) steering = range;
    } else {
        steering = delta + deadZone;
        if (steering > 0) steering = 0;
        if (steering < -range) steering = -range;
    }
    return (s16)steering;
}

void UpdatePadState(void) {
    s32 validationButtons;
    u8 *raw;
    PadState *pad;

    raw = g_PadBuffers;
    pad = &g_PadState;

    pad->status = raw[0];
    g_PadType = g_PadBufferType;
    if (raw[0] != 0) {
        g_PadErrorState = PAD_ERROR_STATE_DISCONNECTED;
        g_PadValidateCountdown = 0x22;
        g_PadErrorHoldBits |= 0x10;
    } else {
        if (g_PadValidateCountdown != 0) {
            g_PadValidateCountdown--;
            if (g_PadBufferType == 0x23) {
                validationButtons =
                    ~(g_PadBufferButtonsLow | (g_PadBufferButtonsHigh << 8));
                if (!(((validationButtons & 0x5000) != 0x5000) &&
                      ((validationButtons & 0xA000) != 0xA000) &&
                      ((validationButtons & 0x1C4) == 0))) {
                    g_PadErrorState = PAD_ERROR_STATE_INVALID_INPUT;
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
        s32 steerRange = GetNegconSteerRange();

        pad->prevHeld = pad->held;
        pad->held = ~((raw[2] << 8) | raw[3]);
        pad->pressed = pad->held & ~pad->prevHeld;
        pad->twist = 0x80;
        if (pad->held & PAD_RIGHT) pad->twist += steerRange;
        if (pad->held & PAD_LEFT) pad->twist -= steerRange;
        pad->buttonI = (pad->held & PAD_CROSS) ? 0x6A : 0;
        pad->buttonII = (pad->held & PAD_SQUARE) ? 0x6A : 0;
        pad->buttonL = (pad->held & PAD_L1) ? 0x6A : 0;
    } else if (raw[1] == 0x23) {
        pad->prevHeld = pad->held;
        pad->held = ~((raw[2] << 8) | raw[3]);
        pad->twist = raw[4];
        pad->buttonI = ClampNegconPressure(raw[5] - g_NegconNeutralI);
        pad->buttonII = ClampNegconPressure(raw[6] - g_NegconNeutralII);
        pad->buttonL = ClampNegconPressure(raw[7] - g_NegconNeutralL);
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
        g_PadRepeatTimer[0] = g_PadRepeatTimer[0] < 0x24
                                  ? g_PadRepeatTimer[0] + 1
                                  : 0x1E;
    } else {
        g_PadRepeatTimer[0] = 0;
    }
    g_PadPrevHeld = pad->held;
    pad->steer = CalibrateNegconSteering(pad->twist);
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
