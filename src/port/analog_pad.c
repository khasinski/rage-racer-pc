#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>

#include "game/input_internal.h"
#include "game/render.h"
#include "game/state.h"
#include "axis_curve.h"
#include "input_device_select.h"
#include "runtime_config.h"

/*
 * Analog gamepad support, presented to the game as a NeGcon.
 *
 * The platform layer only reads gamepad buttons and packs them into a digital
 * PS1 pad, so the sticks and triggers never reach the game. Rather than add a
 * third steering path, this fills the pad buffer with the NeGcon packet
 * UpdatePadState already knows how to decode: buffer[1] is the controller
 * type, [4] the twist, [5] and [6] the analog I and II buttons.
 *
 * Everything downstream then comes for free. The twist runs through the
 * game's own dead zone (g_NegconSteerDeadZone, the OPTIONS "play" setting) and
 * its proportional range (g_NegconSteerRange, the "max twist" setting), so the
 * stick steers by how far it is pushed and both are tunable in the retail
 * menus. UpdatePadState also derives the digital left/right and cross bits
 * from the analog values, so the sticks drive the menus too.
 */

#define NEGCON_ANALOG_MAX 0x6A
#define NEGCON_TWIST_CENTRE 0x80
#define NEGCON_TWIST_MAX 0x7F

/* A NeGcon has no L2, R2 or select cluster. Reporting them released keeps the
 * packet inside the shape UpdatePadState validates for type 0x23. */
#define NEGCON_ABSENT_BUTTONS 0x1C4

/* One axis worth of response shaping, named and ranged as DuckStation has it so
 * a tester can copy values across. */
typedef struct AxisSetup {
    float deadzone, saturation, linearity, scaling;
} AxisSetup;

typedef struct WheelSetup {
    int steeringAxis, throttleAxis, brakeAxis;
    int steeringInverted, pedalsInverted;
    int crossButton, squareButton, circleButton, triangleButton;
    int l1Button, r1Button, startButton;
} WheelSetup;

static float RageAxisSetting(const char *key, float fallback, float low,
                             float high) {
    const char *text = RageRuntimeConfigGet(key);
    char *end;
    float value;
    if (text == NULL || text[0] == '\0') return fallback;
    value = strtof(text, &end);
    if (*end != '\0' || value < low || value > high) {
        fprintf(stderr,
                "rage-port: ignoring %s=%s (expected %.2f..%.2f); using %.2f\n",
                key, text, low, high, fallback);
        return fallback;
    }
    return value;
}

static void RageAxisSetupLoad(AxisSetup *setup, const char *axis) {
    char key[64];
    snprintf(key, sizeof(key), "input.%s_deadzone", axis);
    setup->deadzone = RageAxisSetting(key, 0.0f, 0.0f, 0.99f);
    snprintf(key, sizeof(key), "input.%s_saturation", axis);
    setup->saturation = RageAxisSetting(key, 1.0f, 0.01f, 1.0f);
    snprintf(key, sizeof(key), "input.%s_linearity", axis);
    setup->linearity = RageAxisSetting(key, 0.0f, -2.0f, 2.0f);
    snprintf(key, sizeof(key), "input.%s_scaling", axis);
    setup->scaling = RageAxisSetting(key, 1.0f, 0.01f, 10.0f);
    if (setup->deadzone != 0.0f || setup->saturation != 1.0f ||
        setup->linearity != 0.0f || setup->scaling != 1.0f)
        fprintf(stderr,
                "rage-port: %s response deadzone=%.2f saturation=%.2f linearity=%.2f scaling=%.2f\n",
                axis, setup->deadzone, setup->saturation, setup->linearity,
                setup->scaling);
}

static int RageIntegerSetting(const char *key, int fallback, int low,
                              int high) {
    const char *text = RageRuntimeConfigGet(key);
    char *end;
    long value;
    if (text == NULL || text[0] == '\0') return fallback;
    value = strtol(text, &end, 10);
    if (*end != '\0' || value < low || value > high) {
        fprintf(stderr,
                "rage-port: ignoring %s=%s (expected %d..%d); using %d\n",
                key, text, low, high, fallback);
        return fallback;
    }
    return (int)value;
}

static void RageWheelSetupLoad(WheelSetup *setup) {
    setup->steeringAxis = RageIntegerSetting(
        "input.wheel_steering_axis", 0, 0, 31);
    setup->throttleAxis = RageIntegerSetting(
        "input.wheel_throttle_axis", 2, 0, 31);
    setup->brakeAxis = RageIntegerSetting(
        "input.wheel_brake_axis", 3, 0, 31);
    setup->steeringInverted =
        RageRuntimeConfigEnabled("input.wheel_steering_inverted", NULL);
    setup->pedalsInverted =
        RageRuntimeConfigGet("input.wheel_pedals_inverted") == NULL ||
        RageRuntimeConfigEnabled("input.wheel_pedals_inverted", NULL);
    setup->crossButton = RageIntegerSetting(
        "input.wheel_cross_button", 0, -1, 63);
    setup->squareButton = RageIntegerSetting(
        "input.wheel_square_button", 1, -1, 63);
    setup->circleButton = RageIntegerSetting(
        "input.wheel_circle_button", 2, -1, 63);
    setup->triangleButton = RageIntegerSetting(
        "input.wheel_triangle_button", 3, -1, 63);
    setup->l1Button = RageIntegerSetting(
        "input.wheel_l1_button", 4, -1, 63);
    setup->r1Button = RageIntegerSetting(
        "input.wheel_r1_button", 5, -1, 63);
    setup->startButton = RageIntegerSetting(
        "input.wheel_start_button", 9, -1, 63);
}

/* Shape a raw SDL axis, keeping its sign. */
static float RageAxisShaped(int axis, const AxisSetup *setup) {
    float magnitude = (float)(axis < 0 ? -axis : axis) / 32767.0f;
    float shaped;
    if (magnitude > 1.0f) magnitude = 1.0f;
    shaped = RageAxisCurve(magnitude, setup->deadzone, setup->saturation,
                           setup->linearity, setup->scaling);
    return axis < 0 ? -shaped : shaped;
}

static SDL_Gamepad *RageAnalogFindGamepad(void) {
    static SDL_JoystickID activeId;
    SDL_JoystickID *ids;
    SDL_Gamepad *pad = NULL;
    RageInputDeviceActivity activity[16];
    int count = 0;
    int index;
    int activityCount = 0;

    ids = SDL_GetGamepads(&count);
    if (ids == NULL) return NULL;
    if (count > (int)(sizeof(activity) / sizeof(activity[0])))
        count = (int)(sizeof(activity) / sizeof(activity[0]));
    /* The platform layer opens every gamepad it sees, so look the handles up
     * rather than opening them a second time. Activity, not enumeration order,
     * decides which one controls player one: on a docked Steam Deck the idle
     * built-in controls otherwise always hide a wireless pad. */
    for (index = 0; index < count; index++) {
        SDL_Gamepad *candidate = SDL_GetGamepadFromID(ids[index]);
        int value = 0;
        int axis;
        int button;
        if (candidate == NULL) continue;
        for (axis = 0; axis < SDL_GAMEPAD_AXIS_COUNT; axis++) {
            int sample = SDL_GetGamepadAxis(
                candidate, (SDL_GamepadAxis)axis);
            if (sample < 0) sample = -sample;
            if (sample > value) value = sample;
        }
        for (button = 0; button < SDL_GAMEPAD_BUTTON_COUNT; button++) {
            if (SDL_GetGamepadButton(candidate, (SDL_GamepadButton)button)) {
                value = 32767;
                break;
            }
        }
        activity[activityCount].id = (unsigned int)ids[index];
        activity[activityCount].activity = value;
        activityCount++;
    }
    activeId = (SDL_JoystickID)RageSelectActiveInputDevice(
        activity, (size_t)activityCount, (unsigned int)activeId, 4096);
    if (activeId != 0) pad = SDL_GetGamepadFromID(activeId);
    SDL_free(ids);
    return pad;
}

static unsigned int RageGamepadButtons(SDL_Gamepad *pad) {
    unsigned int held = 0;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_UP))
        held |= PAD_UP;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT))
        held |= PAD_RIGHT;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_DOWN))
        held |= PAD_DOWN;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_LEFT))
        held |= PAD_LEFT;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_SOUTH))
        held |= PAD_CROSS;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_WEST))
        held |= PAD_SQUARE;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_EAST))
        held |= PAD_CIRCLE;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_NORTH))
        held |= PAD_TRIANGLE;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER))
        held |= PAD_L1;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER))
        held |= PAD_R1;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_START))
        held |= PAD_START;
    return held;
}

static SDL_Joystick *RageAnalogFindWheel(void) {
    static SDL_Joystick *wheel;
    SDL_JoystickID *ids;
    int count = 0;
    int index;

    if (wheel != NULL && SDL_JoystickConnected(wheel)) return wheel;
    if (wheel != NULL) {
        SDL_CloseJoystick(wheel);
        wheel = NULL;
    }
    ids = SDL_GetJoysticks(&count);
    if (ids == NULL) return NULL;
    for (index = 0; index < count && wheel == NULL; index++) {
        if (!SDL_IsGamepad(ids[index])) wheel = SDL_OpenJoystick(ids[index]);
    }
    SDL_free(ids);
    return wheel;
}

static int RageWheelButton(SDL_Joystick *wheel, int button) {
    return button >= 0 && button < SDL_GetNumJoystickButtons(wheel) &&
           SDL_GetJoystickButton(wheel, button);
}

static unsigned int RageWheelButtons(SDL_Joystick *wheel,
                                     const WheelSetup *setup) {
    unsigned int held = 0;
    Uint8 hat = SDL_GetNumJoystickHats(wheel) > 0
                    ? SDL_GetJoystickHat(wheel, 0)
                    : SDL_HAT_CENTERED;
    if (hat & SDL_HAT_UP) held |= PAD_UP;
    if (hat & SDL_HAT_RIGHT) held |= PAD_RIGHT;
    if (hat & SDL_HAT_DOWN) held |= PAD_DOWN;
    if (hat & SDL_HAT_LEFT) held |= PAD_LEFT;
    if (RageWheelButton(wheel, setup->crossButton)) held |= PAD_CROSS;
    if (RageWheelButton(wheel, setup->squareButton)) held |= PAD_SQUARE;
    if (RageWheelButton(wheel, setup->circleButton)) held |= PAD_CIRCLE;
    if (RageWheelButton(wheel, setup->triangleButton)) held |= PAD_TRIANGLE;
    if (RageWheelButton(wheel, setup->l1Button)) held |= PAD_L1;
    if (RageWheelButton(wheel, setup->r1Button)) held |= PAD_R1;
    if (RageWheelButton(wheel, setup->startButton)) held |= PAD_START;
    return held;
}

static int RageWheelAxis(SDL_Joystick *wheel, int axis) {
    if (axis < 0 || axis >= SDL_GetNumJoystickAxes(wheel)) return 0;
    return SDL_GetJoystickAxis(wheel, axis);
}

static float RageWheelPedal(SDL_Joystick *wheel, int axis, int inverted) {
    if (axis < 0 || axis >= SDL_GetNumJoystickAxes(wheel)) return 0.0f;
    return RageJoystickPedalAxis(SDL_GetJoystickAxis(wheel, axis), inverted);
}

void RagePortSampleAnalogPad(void) {
    static int enabled = -1;
    static int wheelAnnounced;
    static SDL_JoystickID announcedPadId;
    static AxisSetup steering, throttle, brake;
    static WheelSetup wheelSetup;
    SDL_Gamepad *pad;
    SDL_Joystick *wheel;
    unsigned int released;
    unsigned int held;
    int analogI, analogII, analogL;
    int twist, range;
    int lx;

    if (enabled < 0) {
        enabled = RageRuntimeConfigGet("input.analog") == NULL
                      ? 1
                      : RageRuntimeConfigEnabled("input.analog", NULL);
        RageAxisSetupLoad(&steering, "steering");
        RageAxisSetupLoad(&throttle, "throttle");
        RageAxisSetupLoad(&brake, "brake");
        RageWheelSetupLoad(&wheelSetup);
    }
    if (!enabled) return;

    wheel = RageRuntimeConfigGet("input.wheel") == NULL ||
                    RageRuntimeConfigEnabled("input.wheel", NULL)
                ? RageAnalogFindWheel()
                : NULL;
    pad = wheel == NULL ? RageAnalogFindGamepad() : NULL;
    if (pad == NULL && wheel == NULL) return;

    if (wheel != NULL && !wheelAnnounced) {
        wheelAnnounced = 1;
        announcedPadId = 0;
        fprintf(stderr, "rage-port: racing wheel active as negcon (%s)\n",
                SDL_GetJoystickName(wheel));
    } else if (pad != NULL &&
               SDL_GetGamepadID(pad) != announcedPadId) {
        wheelAnnounced = 0;
        announcedPadId = SDL_GetGamepadID(pad);
        fprintf(stderr, "rage-port: analog pad active as negcon (%s)\n",
                SDL_GetGamepadName(pad));
    }

    /* A real NeGcon twists across the whole byte whatever the calibration says;
     * OPTIONS then maps that through its twist range and play. Scaling here as
     * well applied the calibration twice, and since UpdatePadState subtracts
     * the play from the result, the smallest range with the largest play left
     * 11 of a possible 113 units of lock. The stick therefore reports full
     * deflection and lets the game's own calibration do its job. */
    lx = wheel != NULL ? RageWheelAxis(wheel, wheelSetup.steeringAxis)
                       : SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX);
    if (wheel != NULL && wheelSetup.steeringInverted) lx = -lx;
    released = ((unsigned int)g_PadBuffers[2] << 8) | g_PadBuffers[3];
    held = ~released;
    if (wheel != NULL) held |= RageWheelButtons(wheel, &wheelSetup);
    if (pad != NULL) held |= RageGamepadButtons(pad);
    range = g_NegconSteerRange[g_NegconMaxTwist];
    twist = RageNegconTwist(RageAxisShaped(lx, &steering),
                            (held & PAD_LEFT) != 0, (held & PAD_RIGHT) != 0,
                            range);

    /* Those four are not ordinary buttons on a NeGcon: UpdatePadState
     * regenerates cross, square and L1 from the analog values, and the pad has
     * no select at all. Reporting them released in the digital word is what
     * the type 0x23 packet is validated against - but it also means a button
     * press only survives if it is folded into the analog value it stands for.
     * Take whichever is further pressed, so the triggers meter the throttle
     * while the face buttons still work as full on. */
    if (wheel != NULL) {
        analogI = (int)(RageAxisCurve(
            RageWheelPedal(wheel, wheelSetup.throttleAxis,
                           wheelSetup.pedalsInverted),
            throttle.deadzone, throttle.saturation, throttle.linearity,
            throttle.scaling) * (float)NEGCON_ANALOG_MAX);
        analogII = (int)(RageAxisCurve(
            RageWheelPedal(wheel, wheelSetup.brakeAxis,
                           wheelSetup.pedalsInverted),
            brake.deadzone, brake.saturation, brake.linearity, brake.scaling) *
            (float)NEGCON_ANALOG_MAX);
    } else {
        analogI = (int)(RageAxisShaped(
            SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER), &throttle) *
            (float)NEGCON_ANALOG_MAX);
        analogII = (int)(RageAxisShaped(
            SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER), &brake) *
            (float)NEGCON_ANALOG_MAX);
    }
    analogL = 0;
    if (held & PAD_CROSS) analogI = NEGCON_ANALOG_MAX;
    if (held & PAD_SQUARE) analogII = NEGCON_ANALOG_MAX;
    if (held & PAD_L1) analogL = NEGCON_ANALOG_MAX;

    released = ~held & 0xFFFFU;
    released |= NEGCON_ABSENT_BUTTONS;

    g_PadBuffers[0] = 0;
    g_PadBuffers[1] = 0x23;
    g_PadBuffers[2] = (u8)(released >> 8);
    g_PadBuffers[3] = (u8)released;
    g_PadBuffers[4] = (u8)twist;
    g_PadBuffers[5] = (u8)(g_NegconNeutralI + analogI);
    g_PadBuffers[6] = (u8)(g_NegconNeutralII + analogII);
    g_PadBuffers[7] = (u8)(g_NegconNeutralL + analogL);
}
