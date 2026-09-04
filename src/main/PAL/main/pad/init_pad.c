#include <libapi.h>

#include "game/diagnostics.h"
#include "game/state.h"
#include "game/input_internal.h"
#include "game/render.h"
#include <stdlib.h>
#include <stdio.h>

/* Attach the two retail 0x28-byte pad buffers to the host input backend. */
void GameInitPad(void) {
    (void)InitPAD((char *)g_PadBuffers, PAD_PORT_BUFFER_SIZE,
                  (char *)g_PadBuffers + PAD_PORT_BUFFER_SIZE,
                  PAD_PORT_BUFFER_SIZE);
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
    for (i = 0; i < CONTROLLER_MAPPING_BUTTON_COUNT; i++) {
        g_PadButtonMapping[i] = g_PadButtonPresets[mapping0][i];
        g_PadButtonMapping[CONTROLLER_MAPPING_BUTTON_COUNT + i] =
            g_NegconButtonPresets[mapping1][i];
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
    s32 play = NegconCalibrationIndex(g_NegconSteerPlay);
    s32 neutral = g_NegconSteerNeutral + 0x80;
    s32 delta = twist - neutral;
    s32 deadZone = g_NegconSteerDeadZone[play][0];
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

static u16 DecodeHeldButtons(const u8 *raw) {
    return (u16)~((raw[2] << 8) | raw[3]);
}

static void UpdateDigitalPadState(PadState *pad, const u8 *raw) {
    s32 steerRange = GetNegconSteerRange();

    pad->prevHeld = pad->held;
    pad->held = DecodeHeldButtons(raw);
    pad->pressed = pad->held & ~pad->prevHeld;
    pad->twist = 0x80;
    if (pad->held & PAD_RIGHT) pad->twist += steerRange;
    if (pad->held & PAD_LEFT) pad->twist -= steerRange;
    pad->buttonI = (pad->held & PAD_CROSS) ? 0x6A : 0;
    pad->buttonII = (pad->held & PAD_SQUARE) ? 0x6A : 0;
    pad->buttonL = (pad->held & PAD_L1) ? 0x6A : 0;
}

static void UpdateNegconPadState(PadState *pad, const u8 *raw) {
    pad->prevHeld = pad->held;
    pad->held = DecodeHeldButtons(raw);
    pad->twist = raw[4];
    pad->buttonI = ClampNegconPressure(raw[5] - g_NegconNeutralI);
    pad->buttonII = ClampNegconPressure(raw[6] - g_NegconNeutralII);
    pad->buttonL = ClampNegconPressure(raw[7] - g_NegconNeutralL);
    if (pad->twist >= 0xA3) pad->held |= PAD_RIGHT;
    if (pad->twist < 0x5E) pad->held |= PAD_LEFT;
    if (pad->buttonI >= 0x36) pad->held |= PAD_CROSS;
    if (pad->buttonII >= 0x36) pad->held |= PAD_SQUARE;
    if (pad->buttonL >= 0x36) pad->held |= PAD_L1;
    pad->pressed = pad->held & ~pad->prevHeld;
}

static void ClearInvalidPadState(PadState *pad) {
    if (g_PadErrorState == PAD_ERROR_STATE_NONE) {
        g_PadErrorState = PAD_ERROR_STATE_INVALID_INPUT;
    }
    pad->status = 1;
    pad->type = 0;
    pad->held = 0;
    pad->pressed = 0;
    pad->twist = 0x80;
    pad->buttonI = 0;
    pad->buttonII = 0;
    pad->buttonL = 0;
}

enum {
    PAD_VALIDATION_RETRY_FRAMES = 0x22,
    PAD_ERROR_HOLD_MASK = 0x10,
    NEGCON_DIGITAL_AXIS_MASK = PAD_SELECT | PAD_L1 | PAD_CROSS | PAD_SQUARE,
};

static s32 HasInvalidNegconButtons(u16 held) {
    return (held & (PAD_UP | PAD_DOWN)) == (PAD_UP | PAD_DOWN) ||
           (held & (PAD_LEFT | PAD_RIGHT)) == (PAD_LEFT | PAD_RIGHT) ||
           (held & NEGCON_DIGITAL_AXIS_MASK) != 0;
}

static void StartPadValidationError(PadErrorState error) {
    g_PadErrorState = error;
    g_PadValidateCountdown = PAD_VALIDATION_RETRY_FRAMES;
    g_PadErrorHoldBits |= PAD_ERROR_HOLD_MASK;
}

static void ValidatePadPacket(const u8 *raw, PadState *pad) {
    if (raw[0] != 0) {
        StartPadValidationError(PAD_ERROR_STATE_DISCONNECTED);
    } else if (g_PadValidateCountdown != 0) {
        g_PadValidateCountdown--;
        if (raw[1] == PAD_TYPE_NEGCON &&
            HasInvalidNegconButtons(DecodeHeldButtons(raw))) {
            StartPadValidationError(PAD_ERROR_STATE_INVALID_INPUT);
        }
    }

    g_PadErrorHoldBits >>= 1;
    if (g_PadErrorHoldBits != 0) {
        pad->type = 0;
    } else {
        g_PadErrorState = PAD_ERROR_STATE_NONE;
    }
}

enum {
    PAD_REPEAT_INITIAL_DELAY = 30,
    PAD_REPEAT_CYCLE_END = 36,
};

static void UpdatePadAutoRepeat(PadState *pad) {
    pad->pressedRepeat = pad->held & ~g_PadPrevHeld;
    if (pad->held != 0 && pad->held == g_PadPrevHeld) {
        if (g_PadRepeatTimer == PAD_REPEAT_INITIAL_DELAY) {
            pad->pressedRepeat |= pad->held;
        }
        g_PadRepeatTimer = g_PadRepeatTimer < PAD_REPEAT_CYCLE_END
                               ? g_PadRepeatTimer + 1
                               : PAD_REPEAT_INITIAL_DELAY;
    } else {
        g_PadRepeatTimer = 0;
    }
    g_PadPrevHeld = pad->held;
}

static void PublishPadState(PadState *pad) {
    pad->steer = CalibrateNegconSteering(pad->twist);
    g_PadHeld = pad->held;
    g_PadPressed = pad->pressed;
    g_PadPressedRepeat = pad->pressedRepeat;
    g_NegconAnalogI = pad->buttonI;
    g_NegconAnalogII = pad->buttonII;
    g_NegconAnalogL = pad->buttonL;
    g_NegconSteer = pad->steer;
}

void UpdatePadState(void) {
    u8 *raw = g_PadBuffers;
    PadState *pad = &g_PadState;

    pad->status = raw[0];
    pad->type = raw[1];
    g_PadType = raw[1];
    ValidatePadPacket(raw, pad);

    if (pad->type == PAD_TYPE_DIGITAL) {
        UpdateDigitalPadState(pad, raw);
    } else if (pad->type == PAD_TYPE_NEGCON) {
        UpdateNegconPadState(pad, raw);
    } else {
        ClearInvalidPadState(pad);
    }

    UpdatePadAutoRepeat(pad);
    PublishPadState(pad);
    if (DiagnosticsEnabled("input.debug") &&
        (pad->held != 0 || pad->pressed != 0)) {
        printf("pad input: raw=%02x,%02x,%02x,%02x type=%02x held=%04x pressed=%04x\n",
               raw[0], raw[1], raw[2], raw[3], g_PadType,
               (u16)pad->held, (u16)pad->pressed);
    }
}
