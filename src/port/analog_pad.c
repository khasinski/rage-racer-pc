#include <SDL3/SDL.h>
#include <stdio.h>

#include "game/input_internal.h"
#include "game/render.h"
#include "game/state.h"
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

/* A NeGcon has no L2, R2 or select cluster. Reporting them released keeps the
 * packet inside the shape UpdatePadState validates for type 0x23. */
#define NEGCON_ABSENT_BUTTONS 0x1C4

static int RageAnalogScale(int axis, int range) {
    long scaled;
    if (axis < 0) axis = 0;
    scaled = ((long)axis * range) / 32767L;
    if (scaled > range) scaled = range;
    return (int)scaled;
}

static SDL_Gamepad *RageAnalogFindGamepad(void) {
    SDL_JoystickID *ids;
    SDL_Gamepad *pad = NULL;
    int count = 0;
    int index;

    ids = SDL_GetGamepads(&count);
    if (ids == NULL) return NULL;
    /* The platform layer opens every gamepad it sees, so look the handle up
     * rather than opening a second one. */
    for (index = 0; index < count && pad == NULL; index++) {
        pad = SDL_GetGamepadFromID(ids[index]);
    }
    SDL_free(ids);
    return pad;
}

void RagePortSampleAnalogPad(void) {
    static int enabled = -1;
    static int announced;
    SDL_Gamepad *pad;
    unsigned int released;
    unsigned int held;
    int analogI, analogII, analogL;
    int twist, deflection, range;
    int lx;

    if (enabled < 0) {
        enabled = RageRuntimeConfigGet("input.analog") == NULL
                      ? 1
                      : RageRuntimeConfigEnabled("input.analog", NULL);
    }
    if (!enabled) return;

    pad = RageAnalogFindGamepad();
    if (pad == NULL) return;

    if (!announced) {
        announced = 1;
        fprintf(stderr, "rage-port: analog pad active as negcon (%s)\n",
                SDL_GetGamepadName(pad));
    }

    /* Scale by the game's own twist range rather than the full byte, so the
     * OPTIONS "max twist" setting means the same thing for the stick as it
     * does for a real NeGcon. */
    range = g_NegconSteerRange[g_NegconMaxTwist];
    if (range <= 0) range = 127;

    lx = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX);
    deflection = (int)(((long)lx * range) / 32767L);

    /* The NeGcon steering branch of UpdateCarBodyRoll reads only the twist, so
     * a d-pad press would otherwise stop steering the moment a stick is
     * present. Fold the d-pad in at full deflection and keep whichever input
     * is pushed further, so either one steers and neither cancels the other. */
    released = ((unsigned int)g_PadBuffers[2] << 8) | g_PadBuffers[3];
    held = ~released;
    if ((held & PAD_LEFT) && deflection > -range) deflection = -range;
    if ((held & PAD_RIGHT) && deflection < range) deflection = range;

    twist = NEGCON_TWIST_CENTRE + deflection;
    if (twist < 0) twist = 0;
    if (twist > 0xFF) twist = 0xFF;

    /* Those four are not ordinary buttons on a NeGcon: UpdatePadState
     * regenerates cross, square and L1 from the analog values, and the pad has
     * no select at all. Reporting them released in the digital word is what
     * the type 0x23 packet is validated against - but it also means a button
     * press only survives if it is folded into the analog value it stands for.
     * Take whichever is further pressed, so the triggers meter the throttle
     * while the face buttons still work as full on. */
    analogI = RageAnalogScale(
        SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER),
        NEGCON_ANALOG_MAX);
    analogII = RageAnalogScale(
        SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER),
        NEGCON_ANALOG_MAX);
    analogL = 0;
    if (held & PAD_CROSS) analogI = NEGCON_ANALOG_MAX;
    if (held & PAD_SQUARE) analogII = NEGCON_ANALOG_MAX;
    if (held & PAD_L1) analogL = NEGCON_ANALOG_MAX;

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
