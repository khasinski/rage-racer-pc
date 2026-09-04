/*
 * UpdatePadState is the whole of the pad's translation layer: it takes the raw
 * BIOS packet and produces everything the rest of the game reads about input,
 * from the button word down to the analog pressure the throttle is metered
 * with. Until now only the steering calibration slice of it was covered, by
 * negcon_calibration_tests, so a wrong pressure threshold, a wrong synthetic
 * twist or a broken auto-repeat could not fail a test.
 *
 * The thresholds asserted here are the ones the packet formats and the feel of
 * the controls hang on, so each is checked at the value that must pass and the
 * neighbouring value that must not.
 */

#include "common.h"
#include "game/state.h"
#include "game/input_internal.h"
#include "game/render.h"

#include <stdio.h>
#include <string.h>

void UpdatePadState(void);

static char *s_initPadBuffer0;
static char *s_initPadBuffer1;
static int s_initPadLength0;
static int s_initPadLength1;

int InitPAD(char *buf0, int len0, char *buf1, int len1) {
    s_initPadBuffer0 = buf0;
    s_initPadLength0 = len0;
    s_initPadBuffer1 = buf1;
    s_initPadLength1 = len1;
    return 1;
}
int DiagnosticsEnabled(int channel) { (void)channel; return 0; }

/* The pad's live state: plain storage the game refills every frame. */
u8 g_PadBuffers[PAD_BUFFER_SIZE];
PadState g_PadState;
u8 g_PadType;
u16 g_PadButtonMapping[16];
u16 g_PadPrevHeld;
u16 g_PadHeld;
u16 g_PadPressed;
u16 g_PadPressedRepeat;
u8 g_PadRepeatTimer;
s16 g_NegconAnalogI;
s16 g_NegconAnalogII;
s16 g_NegconAnalogL;
s16 g_NegconSteer;

/*
 * Two rows of eight masks each, selected by index. Retail's own presets live
 * in the game's initialised data; what the loader has to get right is which
 * row each preset lands in and that it takes eight of them, so these carry
 * values a mix-up cannot hide.
 */
/* The retail presets come from the port's own state; the mapping test fills
 * them with values of its own before reading them back. */

static int s_failures;

static void Check(const char *what, int got, int wanted) {
    if (got == wanted) return;
    s_failures++;
    printf("FAIL %s: was %d (0x%x), expected %d (0x%x)\n", what, got, got,
           wanted, wanted);
}

/* Start every case from a pad that has just been plugged in, so a leftover
 * repeat timer or error hold cannot carry between them. */
static void Reset(void) {
    memset(g_PadBuffers, 0, sizeof(g_PadBuffers));
    memset(&g_PadState, 0, sizeof(g_PadState));
    g_PadRepeatTimer = 0;
    g_PadPrevHeld = 0;
    g_PadErrorHoldBits = 0;
    g_PadErrorState = PAD_ERROR_STATE_NONE;
    g_PadValidateCountdown = 0;
    g_NegconNeutralI = 0;
    g_NegconNeutralII = 0;
    g_NegconNeutralL = 0;
    g_NegconSteerNeutral = 0;
    g_NegconMaxTwist = 0;
    g_NegconSteerPlay = 0;
}

/* One frame of pad input. `held` is what the player is holding; the packet
 * carries it inverted, which is what the game undoes. */
static void Frame(u8 status, u8 type, u16 held, u8 twist, u8 pressureI,
                  u8 pressureII, u8 pressureL) {
    u16 raw = (u16)~held;
    g_PadBuffers[0] = status;
    g_PadBuffers[1] = type;
    g_PadBuffers[2] = (u8)(raw >> 8);
    g_PadBuffers[3] = (u8)raw;
    g_PadBuffers[4] = twist;
    g_PadBuffers[5] = pressureI;
    g_PadBuffers[6] = pressureII;
    g_PadBuffers[7] = pressureL;
    UpdatePadState();
}

static void Digital(u16 held) { Frame(0, 0x41, held, 0, 0, 0, 0); }
static void Negcon(u8 twist, u8 i, u8 ii, u8 l) {
    Frame(0, 0x23, 0, twist, i, ii, l);
}

/*
 * A digital pad has no analog anything, so the game manufactures it: the d-pad
 * becomes a twist one full range either way, and a face button becomes the
 * pressure a NeGcon reports fully pressed.
 */
static void DigitalPadSynthesisTests(void) {
    int maxTwist;

    for (maxTwist = 0; maxTwist < 4; maxTwist++) {
        int range = g_NegconSteerRange[maxTwist];
        Reset();
        g_NegconMaxTwist = (NegconCalibrationValue)maxTwist;

        Digital(0);
        Check("digital rest twist", g_PadState.twist, 0x80);
        Check("digital packet type", g_PadState.type, PAD_TYPE_DIGITAL);
        Check("digital rest steer", g_PadState.steer, 0);

        Digital(PAD_RIGHT);
        Check("digital right twist", g_PadState.twist, 0x80 + range);
        Digital(PAD_LEFT);
        Check("digital left twist", g_PadState.twist, 0x80 - range);
        /* Pressing both is the same as pressing neither. */
        Digital(PAD_LEFT | PAD_RIGHT);
        Check("digital both twist", g_PadState.twist, 0x80);
    }

    Reset();
    Digital(PAD_CROSS);
    Check("cross pressure", g_PadState.buttonI, 0x6A);
    Check("cross leaves square alone", g_PadState.buttonII, 0);
    Check("cross leaves L1 alone", g_PadState.buttonL, 0);
    Digital(PAD_SQUARE);
    Check("square pressure", g_PadState.buttonII, 0x6A);
    Check("square leaves cross alone", g_PadState.buttonI, 0);
    Digital(PAD_L1);
    Check("L1 pressure", g_PadState.buttonL, 0x6A);
    Digital(0);
    Check("released cross pressure", g_PadState.buttonI, 0);

    Digital(PAD_CROSS);
    Check("held word reaches the game", (int)g_PadHeld, PAD_CROSS);
    Check("held word reaches the pad", g_PadState.held, PAD_CROSS);
    Check("analog throttle reaches the game", g_NegconAnalogI, 0x6A);
}

/*
 * A NeGcon reports pressure and twist as bytes, offset by whatever the
 * calibration screen recorded as the resting position. The game subtracts that
 * offset, refuses a negative result, and past a threshold reports the analog
 * axis as the button it stands for so menus can be driven with it.
 */
static void NegconPressureTests(void) {
    Reset();

    Negcon(0x80, 0x40, 0x30, 0x20);
    Check("NeGcon packet type", g_PadState.type, PAD_TYPE_NEGCON);
    Check("pressure I passes through", g_PadState.buttonI, 0x40);
    Check("pressure II passes through", g_PadState.buttonII, 0x30);
    Check("pressure L passes through", g_PadState.buttonL, 0x20);

    /* The recorded neutral is subtracted, and a reading under it is rest, not
     * a negative pressure. */
    g_NegconNeutralI = 0x10;
    g_NegconNeutralII = 0x10;
    g_NegconNeutralL = 0x10;
    Negcon(0x80, 0x40, 0x08, 0x10);
    Check("neutral subtracted from I", g_PadState.buttonI, 0x30);
    Check("reading under neutral is rest", g_PadState.buttonII, 0);
    Check("reading at neutral is rest", g_PadState.buttonL, 0);
    g_NegconNeutralI = 0;
    g_NegconNeutralII = 0;
    g_NegconNeutralL = 0;

    /* The digital half of a NeGcon packet still carries the buttons it does
     * have, out of both halves of the button word. */
    Frame(0, 0x23, PAD_START | PAD_R1, 0x80, 0, 0, 0);
    Check("negcon holds its low-byte buttons",
          g_PadState.held & (PAD_START | PAD_R1), PAD_START | PAD_R1);
    Frame(0, 0x23, PAD_UP | PAD_TRIANGLE, 0x80, 0, 0, 0);
    Check("negcon holds its high-byte buttons",
          g_PadState.held & (PAD_UP | PAD_TRIANGLE), PAD_UP | PAD_TRIANGLE);
    Check("a newly held button counts as pressed",
          g_PadState.pressed & PAD_UP, PAD_UP);
    Frame(0, 0x23, PAD_UP | PAD_TRIANGLE, 0x80, 0, 0, 0);
    Check("a button held on is not pressed again",
          g_PadState.pressed & PAD_UP, 0);

    /* 0x36 is where an analog press starts counting as the button. */
    Negcon(0x80, 0x35, 0x35, 0x35);
    Check("cross below the press threshold",
          (g_PadState.held & PAD_CROSS) != 0, 0);
    Check("square below the press threshold",
          (g_PadState.held & PAD_SQUARE) != 0, 0);
    Check("L1 below the press threshold", (g_PadState.held & PAD_L1) != 0, 0);
    Negcon(0x80, 0x36, 0x36, 0x36);
    Check("cross at the press threshold",
          (g_PadState.held & PAD_CROSS) != 0, 1);
    Check("square at the press threshold",
          (g_PadState.held & PAD_SQUARE) != 0, 1);
    Check("L1 at the press threshold", (g_PadState.held & PAD_L1) != 0, 1);

    /* Fully pressed is 0x6A, so a reading above it is a miscalibrated pad and
     * gets pulled back rather than reporting more than full throttle. */
    Negcon(0x80, 0x6A, 0x6B, 0xFF);
    Check("full pressure is kept", g_PadState.buttonI, 0x6A);
    Check("one past full is pulled back", g_PadState.buttonII, 0x6A);
    Check("a wild reading is pulled back", g_PadState.buttonL, 0x6A);
}

/* The twist doubles as a d-pad so the menus can be steered with the wheel. */
static void NegconTwistAsDpadTests(void) {
    Reset();

    Negcon(0xA2, 0, 0, 0);
    Check("just under the right threshold",
          (g_PadState.held & PAD_RIGHT) != 0, 0);
    Negcon(0xA3, 0, 0, 0);
    Check("at the right threshold", (g_PadState.held & PAD_RIGHT) != 0, 1);
    Negcon(0x5E, 0, 0, 0);
    Check("at the left threshold", (g_PadState.held & PAD_LEFT) != 0, 0);
    Negcon(0x5D, 0, 0, 0);
    Check("just past the left threshold",
          (g_PadState.held & PAD_LEFT) != 0, 1);
    Negcon(0x80, 0, 0, 0);
    Check("centred twist steers neither way",
          (g_PadState.held & (PAD_LEFT | PAD_RIGHT)) != 0, 0);
}

/*
 * The play deadzone swallows the first units of twist so a wheel that does not
 * quite centre still goes straight, and the range clamps the rest.
 */
static void SteerShapingTests(void) {
    int play;

    for (play = 0; play < 4; play++) {
        int deadZone = g_NegconSteerDeadZone[play][0];
        Reset();
        g_NegconSteerPlay = (NegconCalibrationValue)play;
        g_NegconMaxTwist = 0; /* the narrowest range, 25 units */

        Negcon((u8)(0x80 + deadZone), 0, 0, 0);
        Check("twist inside the play goes straight", g_PadState.steer, 0);
        Negcon((u8)(0x80 + deadZone + 1), 0, 0, 0);
        Check("twist just past the play steers", g_PadState.steer, 1);
        if (deadZone != 0) {
            Negcon((u8)(0x80 - deadZone), 0, 0, 0);
            Check("twist inside the play the other way", g_PadState.steer, 0);
        }
        Negcon((u8)(0x80 - deadZone - 1), 0, 0, 0);
        Check("twist just past the play the other way", g_PadState.steer, -1);

        Negcon(0xFF, 0, 0, 0);
        Check("full twist right clamps to the range", g_PadState.steer,
              g_NegconSteerRange[0]);
        Negcon(0x00, 0, 0, 0);
        Check("full twist left clamps to the range", g_PadState.steer,
              -g_NegconSteerRange[0]);
    }

    /* The recorded resting position shifts the whole axis. */
    Reset();
    g_NegconSteerNeutral = 0x10;
    Negcon(0x90, 0, 0, 0);
    Check("a twist at the recorded neutral goes straight",
          g_PadState.steer, 0);
    Negcon(0x80, 0, 0, 0);
    Check("a centred twist against an offset neutral steers left",
          g_PadState.steer, -0x10);
}

/*
 * Holding a button repeats it, so a held direction walks a menu. The first
 * repeat waits 30 frames and the rest follow every 7.
 */
static void AutoRepeatTests(void) {
    int frame;
    int firstRepeat = 0;
    int secondRepeat = 0;

    Reset();
    Digital(PAD_DOWN);
    Check("a fresh press repeats at once", g_PadState.pressedRepeat, PAD_DOWN);

    for (frame = 1; frame <= 60; frame++) {
        Digital(PAD_DOWN);
        if (g_PadState.pressedRepeat == 0) continue;
        if (firstRepeat == 0) firstRepeat = frame;
        else if (secondRepeat == 0) secondRepeat = frame;
    }
    Check("the first repeat waits thirty frames", firstRepeat, 31);
    Check("the repeats after it come every seven", secondRepeat - firstRepeat,
          7);

    /* Letting go stops the repeat and rearms the wait. */
    Reset();
    for (frame = 0; frame < 40; frame++) Digital(PAD_DOWN);
    Digital(0);
    Check("releasing clears the repeat", g_PadState.pressedRepeat, 0);
    Check("releasing clears the timer", g_PadRepeatTimer, 0);

    /* Changing direction is a fresh press and starts a fresh delay. */
    Digital(PAD_RIGHT);
    Check("a changed button repeats at once", g_PadState.pressedRepeat,
          PAD_RIGHT);
    Check("a changed button rearms the delay", g_PadRepeatTimer, 0);

    /* A damaged timer recovers into the regular repeat cycle. */
    g_PadRepeatTimer = 0xFF;
    Digital(PAD_RIGHT);
    Check("a corrupt repeat timer does not fire", g_PadState.pressedRepeat, 0);
    Check("a corrupt repeat timer recovers", g_PadRepeatTimer, 30);
    Digital(PAD_RIGHT);
    Check("the recovered timer repeats next frame", g_PadState.pressedRepeat,
          PAD_RIGHT);
}

/*
 * A packet the game does not recognise, and a pad that reports itself
 * unplugged, both have to leave the controls in a state that drives nothing.
 */
static void ErrorHandlingTests(void) {
    Reset();
    Frame(0, 0x12, PAD_CROSS, 0xFF, 0xFF, 0xFF, 0xFF);
    Check("an unknown packet reports an error", g_PadErrorState,
          PAD_ERROR_STATE_INVALID_INPUT);
    Check("an unknown packet holds nothing", g_PadState.held, 0);
    Check("an unknown packet centres the twist", g_PadState.twist, 0x80);
    Check("an unknown packet lets go of the throttle", g_PadState.buttonI, 0);
    Check("an unknown packet flags the pad", g_PadState.status, 1);

    Reset();
    Digital(0);
    Check("a connected pad reports no fault", g_PadState.status, 0);
    Check("a connected pad reports its type", g_PadType, 0x41);

    Reset();
    Frame(0xFF, 0x41, PAD_CROSS, 0, 0, 0, 0);
    /* The raw fault byte is taken first but the error path overwrites it, so
     * what the game sees for an unplugged pad is the flag, not the byte. */
    Check("an unplugged pad is flagged", g_PadState.status, 1);
    Check("an unplugged pad has no type", g_PadState.type, 0);
    Check("an unplugged pad reports an error", g_PadErrorState,
          PAD_ERROR_STATE_DISCONNECTED);
    Check("validation leaves the BIOS packet type intact", g_PadBuffers[1],
          PAD_TYPE_DIGITAL);
    Check("an unplugged pad rearms the validation", g_PadValidateCountdown,
          0x22);
    Check("an unplugged pad holds nothing", g_PadState.held, 0);

    /* The error hold outlives the frame that raised it, so one good packet is
     * not enough to hand the controls back. */
    Frame(0, 0x41, PAD_CROSS, 0, 0, 0, 0);
    Check("the frame after an unplug still holds nothing", g_PadState.held, 0);
}

/*
 * A pad that has just been plugged in is not trusted for a while: for as long
 * as the countdown runs, a NeGcon packet claiming something the controller
 * cannot physically report is taken as a bad read and restarts the wait. A
 * NeGcon cannot press two opposite directions at once, and it has no select,
 * square, cross or L1 in its digital word at all, because those are the analog
 * axes.
 */
static void PacketValidationTests(void) {
    static const struct {
        const char *what;
        u16 held;
    } impossible[] = {
        {"up and down together", PAD_UP | PAD_DOWN},
        {"left and right together", PAD_LEFT | PAD_RIGHT},
        {"a select the pad does not have", PAD_SELECT},
        /* The pair is what makes the read bad, whatever else is held with
         * it, so the check has to look at exactly those two bits. */
        {"left and right with another button", PAD_LEFT | PAD_RIGHT | PAD_L2},
        {"up and down with another button", PAD_UP | PAD_DOWN | PAD_R2},
        {"a digital cross that should be analog", PAD_CROSS},
        {"a digital square that should be analog", PAD_SQUARE},
        {"a digital L1 that should be analog", PAD_L1},
    };
    size_t i;

    for (i = 0; i < sizeof(impossible) / sizeof(impossible[0]); i++) {
        Reset();
        g_PadValidateCountdown = 0x10;
        Frame(0, 0x23, impossible[i].held, 0x80, 0, 0, 0);
        Check(impossible[i].what, g_PadErrorState,
              PAD_ERROR_STATE_INVALID_INPUT);
        Check("a bad read restarts the wait", g_PadValidateCountdown, 0x22);
    }

    /* A packet a NeGcon really can send just spends one frame of the wait. */
    Reset();
    g_PadValidateCountdown = 0x10;
    Frame(0, 0x23, PAD_UP | PAD_R1, 0x80, 0, 0, 0);
    Check("a plausible packet is accepted", g_PadErrorState,
          PAD_ERROR_STATE_NONE);
    Check("a plausible packet spends one frame of the wait",
          g_PadValidateCountdown, 0x0F);

    /* The last frame of the wait still validates. */
    Reset();
    g_PadValidateCountdown = 1;
    Frame(0, 0x23, PAD_LEFT | PAD_RIGHT, 0x80, 0, 0, 0);
    Check("the last frame of the wait still checks", g_PadErrorState,
          PAD_ERROR_STATE_INVALID_INPUT);

    /* Once the pad is trusted, nothing is second-guessed. */
    Reset();
    Frame(0, 0x23, PAD_LEFT | PAD_RIGHT, 0x80, 0, 0, 0);
    Check("a trusted pad is not second-guessed", g_PadErrorState,
          PAD_ERROR_STATE_NONE);
}

/*
 * The mapping table the game reads is two rows: the pad's eight masks and the
 * NeGcon's eight after them. Both selections are copied by one loop, so the
 * mistake to guard against is a row landing on the other one's half.
 */
static void ButtonMappingTests(void) {
    int i;

    for (i = 0; i < 64; i++) {
        g_PadButtonPresets[i / CONTROLLER_MAPPING_BUTTON_COUNT]
                          [i % CONTROLLER_MAPPING_BUTTON_COUNT] =
            (u16)(0x100 + i);
        g_NegconButtonPresets[i / CONTROLLER_MAPPING_BUTTON_COUNT]
                             [i % CONTROLLER_MAPPING_BUTTON_COUNT] =
            (u16)(0x200 + i);
    }
    memset(g_PadButtonMapping, 0xFF, sizeof(g_PadButtonMapping));

    LoadPadButtonMapping(2, 5);
    for (i = 0; i < 8; i++) {
        Check("pad row", g_PadButtonMapping[i], 0x100 + 2 * 8 + i);
        Check("negcon row", g_PadButtonMapping[8 + i], 0x200 + 5 * 8 + i);
    }

    LoadPadButtonMapping(-1, CONTROLLER_MAPPING_COUNT);
    for (i = 0; i < 8; i++) {
        Check("low pad mapping clamps", g_PadButtonMapping[i], 0x100 + i);
        Check("high negcon mapping clamps", g_PadButtonMapping[8 + i],
              0x200 + CONTROLLER_MAPPING_LAST * 8 + i);
    }

    /* The saved selections are what ApplyPadButtonMapping reinstates. */
    g_PadMappingIndex = 1;
    g_NegconMappingIndex = 3;
    memset(g_PadButtonMapping, 0xFF, sizeof(g_PadButtonMapping));
    ApplyPadButtonMapping();
    for (i = 0; i < 8; i++) {
        Check("reinstated pad row", g_PadButtonMapping[i], 0x100 + 1 * 8 + i);
        Check("reinstated negcon row", g_PadButtonMapping[8 + i],
              0x200 + 3 * 8 + i);
    }
}

static void PadInitializationTests(void) {
    s_initPadBuffer0 = NULL;
    s_initPadBuffer1 = NULL;
    s_initPadLength0 = 0;
    s_initPadLength1 = 0;

    GameInitPad();

    Check("first BIOS packet starts at the pad buffer",
          s_initPadBuffer0 == (char *)g_PadBuffers, 1);
    Check("second BIOS packet follows the first",
          s_initPadBuffer1 ==
              (char *)g_PadBuffers + PAD_PORT_BUFFER_SIZE,
          1);
    Check("first BIOS packet has the retail size", s_initPadLength0,
          PAD_PORT_BUFFER_SIZE);
    Check("second BIOS packet has the retail size", s_initPadLength1,
          PAD_PORT_BUFFER_SIZE);
}

int main(void) {
    DigitalPadSynthesisTests();
    NegconPressureTests();
    NegconTwistAsDpadTests();
    SteerShapingTests();
    AutoRepeatTests();
    ErrorHandlingTests();
    PacketValidationTests();
    ButtonMappingTests();
    PadInitializationTests();

    if (s_failures != 0) {
        printf("%d pad state assertion(s) failed\n", s_failures);
        return 1;
    }
    printf("the pad translation layer holds every threshold it is built on\n");
    return 0;
}
