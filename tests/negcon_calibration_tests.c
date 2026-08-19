/*
 * Calibrating a NeGcon must not cost the stick its lock.
 *
 * The port turns a gamepad stick into the twist byte a NeGcon reports, and the
 * game maps that byte through the two settings the OPTIONS screen calibrates:
 * the twist range ("max twist") and the dead zone ("play"). The port used to
 * apply the range itself as well, so a calibration landed twice and the
 * smallest range with the largest play left a fraction of the lock it should
 * have had.
 *
 * The port's half is RageNegconTwist and the game's half is UpdatePadState, and
 * a fault in either only shows in the two together, so this drives the real
 * UpdatePadState with the real twist byte the port produces. It asks for the
 * one thing a calibration must never take away: pushing the stick all the way
 * still reaches full lock, at every one of the sixteen combinations the two
 * settings can take.
 */

#include "common.h"
#include "game/state.h"
#include "game/input_internal.h"
#include "game/render.h"

#include <stdio.h>
#include <string.h>

#include "axis_curve.h"

void UpdatePadState(void);

/* UpdatePadState reaches these; nothing it does on the path under test uses
 * them, and the host backend they belong to has no place in a unit test. */
void InitPad(void *buf0, int len0, void *buf1, int len1) {
    (void)buf0; (void)len0; (void)buf1; (void)len1;
}
int RageDiagnosticsEnabled(int channel) { (void)channel; return 0; }

/*
 * The pad's own live state. It is plain storage that the game fills in every
 * frame, so standing it up here costs the test nothing. The two tables a
 * calibration actually selects from, g_NegconSteerRange and
 * g_NegconSteerDeadZone, are the real ones: they come from host_state.c, which
 * is where a wrong value would hide.
 */
u8 g_PadBuffers[0x50];
PadState g_PadState;
u8 g_PadType;
u16 g_PadButtonMapping[16];
u16 g_PadPrevHeld;
volatile u16 g_PadHeld;
u16 g_PadPressed;
u16 g_PadPressedRepeat;
u8 g_PadRepeatTimer[4];
s16 g_NegconAnalogI;
s16 g_NegconAnalogII;
s16 g_NegconAnalogL;
s16 g_NegconSteer;

static int s_failures;

static void Check(int condition, const char *what, int maxTwist, int play,
                  int got, int wanted) {
    if (condition) return;
    s_failures++;
    printf("FAIL max_twist=%d play=%d: %s was %d, expected %d\n", maxTwist, play,
           what, got, wanted);
}

/* Present one NeGcon report to the game and let it map it. */
static void Report(int twist) {
    memset(g_PadBuffers, 0, 0x28);
    g_PadBuffers[0] = 0;    /* connected */
    g_PadBuffers[1] = 0x23; /* NeGcon */
    g_PadBuffers[2] = 0xFF; /* no button held; the game inverts these */
    g_PadBuffers[3] = 0xFF;
    g_PadBuffers[4] = (u8)twist;
    g_PadBuffers[5] = 0;
    g_PadBuffers[6] = 0;
    g_PadBuffers[7] = 0;
    g_PadValidateCountdown = 0;
    g_PadErrorHoldBits = 0;
    UpdatePadState();
}

int main(void) {
    int maxTwist, play;

    g_NegconNeutralI = 0;
    g_NegconNeutralII = 0;
    g_NegconNeutralL = 0;

    for (maxTwist = 0; maxTwist < 4; maxTwist++) {
        for (play = 0; play < 4; play++) {
            int range = g_NegconSteerRange[maxTwist];
            int deadZone = g_NegconSteerDeadZone[play][0];
            int twist;

            g_NegconMaxTwist = maxTwist;
            g_NegconSteerPlay = play;
            /* What the calibration screen records for a stick that rests in the
             * middle: g_NegconAxisSteer - 128, and the port centres at 128. */
            g_NegconSteerNeutral = 0;

            /* At rest the car must go straight, whatever the play is. */
            Report(RageNegconTwist(0.0f, 0, 0, range));
            Check(g_PadState.steer == 0, "resting steer", maxTwist, play,
                  g_PadState.steer, 0);

            /* Pushed all the way, either way, the lock is the range. */
            twist = RageNegconTwist(1.0f, 0, 0, range);
            Report(twist);
            Check(g_PadState.steer == range, "stick right", maxTwist, play,
                  g_PadState.steer, range);

            twist = RageNegconTwist(-1.0f, 0, 0, range);
            Report(twist);
            Check(g_PadState.steer == -range, "stick left", maxTwist, play,
                  g_PadState.steer, -range);

            /* The d-pad keeps retail's synthetic twist, which is the range
             * exactly, so the play still eats into it the way it always did.
             * Digital steering is not what calibration broke and must not
             * change. */
            twist = RageNegconTwist(0.0f, 0, 1, range);
            Report(twist);
            Check(g_PadState.steer == range - deadZone, "d-pad right", maxTwist,
                  play, g_PadState.steer, range - deadZone);

            /* Half a push is half the byte, and the game takes the play off it,
             * so a calibration with more play reaches less. That is the
             * setting doing its job rather than the port double-counting it. */
            twist = RageNegconTwist(0.5f, 0, 0, range);
            Report(twist);
            {
                int wanted = 0x7F / 2 - deadZone;
                if (wanted > range) wanted = range;
                if (wanted < 0) wanted = 0;
                Check(g_PadState.steer == wanted, "stick half right", maxTwist,
                      play, g_PadState.steer, wanted);
            }
        }
    }

    if (s_failures != 0) {
        printf("%d NeGcon calibration checks failed\n", s_failures);
        return 1;
    }
    printf("every NeGcon calibration reaches full lock from the stick\n");
    return 0;
}
