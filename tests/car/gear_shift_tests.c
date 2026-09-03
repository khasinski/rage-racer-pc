/*
 * The gearbox.
 *
 * ShiftPlayerGears came out of the middle of UpdatePlayerCar, where it was
 * unreachable by anything smaller than a whole race. A smoke run never gets
 * the car past third, so the tall gears, the top-gear stop, the manual box and
 * the anti-hunting cooldown were all decided by code nothing exercised.
 *
 * It reads a car, the pad, and the car's own shift-point table, and writes the
 * gear, the clutch and two timers. That is small enough to ask directly.
 */

#include "common.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/state.h"

#include <stdio.h>
#include <string.h>

GameCarSpec *g_CarSpec;
u16 g_PadPressed;
u16 g_PadButtonMapping[16];
s32 g_AutoShiftCooldown;
s16 g_SteerHoldFrames;

/* The two buttons, at the slots a normal pad puts them. */
#define SHIFT_UP 0x0008
#define SHIFT_DOWN 0x0004

static GameCarSpec s_spec;
static PlayerCarRuntime s_car;
static int s_failures;

static void Check(int condition, const char *what, s32 got, s32 wanted) {
    if (condition) return;
    printf("FAIL %s: got %d, expected %d\n", what, got, wanted);
    s_failures++;
}

/* A six-speed box that shifts up at 1000 km/h intervals and down at 800. */
static void BuildSpec(void) {
    int i;

    memset(&s_spec, 0, sizeof(s_spec));
    s_spec.topGear = 6;
    for (i = 0; i < 6; i++) {
        s_spec.shiftPoints[i].upshiftSpeed = (s16)(1000 * (i + 1));
        s_spec.shiftPoints[i].downshiftSpeed = (s16)(800 * i);
    }
    g_CarSpec = &s_spec;

    memset(g_PadButtonMapping, 0, sizeof(g_PadButtonMapping));
    g_PadButtonMapping[4] = SHIFT_UP;
    g_PadButtonMapping[5] = SHIFT_DOWN;
    /* A NeGcon's two are eight slots along. */
    g_PadButtonMapping[12] = 0x0100;
    g_PadButtonMapping[13] = 0x0200;
}

static void Place(int manual, s32 gear, s32 speed) {
    memset(&s_car, 0, sizeof(s_car));
    s_car.drive.manual = (s16)manual;
    s_car.drive.gear = (s16)gear;
    s_car.speed = speed;
    s_car.drive.clutch = 0;
    s_car.verticalMotionState = CAR_VERTICAL_GROUNDED;
    s_car.drive.motionState = CAR_MOTION_DRIVING;
    g_PadPressed = 0;
    g_AutoShiftCooldown = 0;
    g_SteerHoldFrames = 7;
}

static void ManualTests(void) {
    /* The buttons move one gear each. */
    Place(1, 3, 2000);
    g_PadPressed = SHIFT_UP;
    ShiftPlayerGears(&s_car, 0);
    Check(s_car.drive.gear == 4, "manual up", s_car.drive.gear, 4);
    Check(g_SteerHoldFrames == 0, "shifting clears the steering hold",
          g_SteerHoldFrames, 0);

    Place(1, 3, 2000);
    g_PadPressed = SHIFT_DOWN;
    ShiftPlayerGears(&s_car, 0);
    Check(s_car.drive.gear == 2, "manual down", s_car.drive.gear, 2);

    Place(1, 3, 2000);
    g_PadPressed = SHIFT_UP | SHIFT_DOWN;
    ShiftPlayerGears(&s_car, 0);
    Check(s_car.drive.gear == 3, "simultaneous shifts cancel in order",
          s_car.drive.gear, 3);

    /* Neither goes past its end of the box. */
    Place(1, 6, 2000);
    g_PadPressed = SHIFT_UP;
    ShiftPlayerGears(&s_car, 0);
    Check(s_car.drive.gear == 6, "no gear above the top", s_car.drive.gear, 6);

    Place(1, 1, 2000);
    g_PadPressed = SHIFT_DOWN;
    ShiftPlayerGears(&s_car, 0);
    Check(s_car.drive.gear == 1, "no gear below first", s_car.drive.gear, 1);

    /* Mid-shift the box will not take another one. */
    Place(1, 3, 2000);
    s_car.drive.clutch = 5;
    g_PadPressed = SHIFT_UP;
    ShiftPlayerGears(&s_car, 0);
    Check(s_car.drive.gear == 3, "no upshift while the clutch is out",
          s_car.drive.gear, 3);

    /* Downshifting is allowed mid-shift, which is retail's asymmetry, not a
     * slip here: only the upshift checks the clutch. */
    Place(1, 3, 2000);
    s_car.drive.clutch = 5;
    g_PadPressed = SHIFT_DOWN;
    ShiftPlayerGears(&s_car, 0);
    Check(s_car.drive.gear == 2, "downshift ignores the clutch",
          s_car.drive.gear, 2);

    /* A NeGcon's buttons are at the other pair of slots, and the normal pad's
     * do nothing on one. */
    Place(1, 3, 2000);
    g_PadPressed = 0x0100;
    ShiftPlayerGears(&s_car, 1);
    Check(s_car.drive.gear == 4, "negcon up", s_car.drive.gear, 4);

    Place(1, 3, 2000);
    g_PadPressed = SHIFT_UP;
    ShiftPlayerGears(&s_car, 1);
    Check(s_car.drive.gear == 3, "a pad button does nothing on a negcon",
          s_car.drive.gear, 3);

    Place(1, 0, 0);
    ShiftPlayerGears(&s_car, 0);
    Check(s_car.drive.gear == 1, "invalid low gear is repaired",
          s_car.drive.gear, 1);

    Place(1, 7, 0);
    ShiftPlayerGears(&s_car, 0);
    Check(s_car.drive.gear == 6, "invalid high gear is repaired",
          s_car.drive.gear, 6);

    s_spec.topGear = 3;
    Place(1, 5, 0);
    ShiftPlayerGears(&s_car, 0);
    Check(s_car.drive.gear == 3, "gear is capped by the configured gearbox",
          s_car.drive.gear, 3);

    s_spec.topGear = 0;
    Place(1, 3, 0);
    ShiftPlayerGears(&s_car, 0);
    Check(s_car.drive.gear == 1, "invalid low top gear falls back to first",
          s_car.drive.gear, 1);

    s_spec.topGear = 7;
    Place(1, 7, 0);
    ShiftPlayerGears(&s_car, 0);
    Check(s_car.drive.gear == 6, "invalid high top gear falls back to sixth",
          s_car.drive.gear, 6);
    s_spec.topGear = CAR_FORWARD_GEAR_COUNT;
}

static void AutomaticTests(void) {
    Place(0, 0, 500);
    ShiftPlayerGears(&s_car, 0);
    Check(s_car.drive.gear == 1, "auto repairs gear before table lookup",
          s_car.drive.gear, 1);

    /* Above the current gear's upshift speed it takes the next one. */
    Place(0, 1, 1500);
    ShiftPlayerGears(&s_car, 0);
    Check(s_car.drive.gear == 2, "auto up", s_car.drive.gear, 2);
    Check(g_AutoShiftCooldown == 25 - 1, "and starts the cooldown",
          g_AutoShiftCooldown, 24);

    /* Below the current gear's downshift speed it drops one. */
    Place(0, 4, 2000);
    ShiftPlayerGears(&s_car, 0);
    Check(s_car.drive.gear == 3, "auto down", s_car.drive.gear, 3);

    /* Between the two it stays where it is. */
    Place(0, 3, 2500);
    ShiftPlayerGears(&s_car, 0);
    Check(s_car.drive.gear == 3, "auto holds between the two speeds",
          s_car.drive.gear, 3);

    /* Top gear has nothing above it however fast the car goes. */
    Place(0, 6, 100000);
    ShiftPlayerGears(&s_car, 0);
    Check(s_car.drive.gear == 6, "auto stops at the top gear",
          s_car.drive.gear, 6);

    /* The cooldown blocks both directions while it runs. */
    Place(0, 1, 1500);
    g_AutoShiftCooldown = 5;
    ShiftPlayerGears(&s_car, 0);
    Check(s_car.drive.gear == 1, "the cooldown blocks an upshift",
          s_car.drive.gear, 1);
    Check(g_AutoShiftCooldown == 4, "and counts down", g_AutoShiftCooldown, 4);

    Place(0, 4, 2000);
    g_AutoShiftCooldown = 5;
    ShiftPlayerGears(&s_car, 0);
    Check(s_car.drive.gear == 4, "the cooldown blocks a downshift",
          s_car.drive.gear, 4);

    /* Braking hard runs it out twice as fast, so a car slowing down gets its
     * gears sooner. */
    Place(0, 4, 2000);
    g_AutoShiftCooldown = 10;
    s_car.drive.brakeInput = 129;
    ShiftPlayerGears(&s_car, 0);
    Check(g_AutoShiftCooldown == 8, "braking halves the wait",
          g_AutoShiftCooldown, 8);

    Place(0, 4, 2000);
    g_AutoShiftCooldown = 10;
    s_car.drive.brakeInput = 128;
    ShiftPlayerGears(&s_car, 0);
    Check(g_AutoShiftCooldown == 9, "just short of hard braking does not",
          g_AutoShiftCooldown, 9);

    /* Coming to a stop drops straight to first and frees the clutch. */
    Place(0, 5, 0);
    s_car.drive.clutch = 4;
    g_AutoShiftCooldown = 10;
    ShiftPlayerGears(&s_car, 0);
    Check(s_car.drive.gear == 1, "stopping drops to first", s_car.drive.gear,
          1);
    Check(s_car.drive.clutch == 0, "and lets the clutch in",
          s_car.drive.clutch, 0);
    Check(g_AutoShiftCooldown == 0, "and clears the wait", g_AutoShiftCooldown,
          0);

    /*
     * Except on the grid. The guard is narrower than it looks: it only stops
     * the drop straight to first, so a car sitting on the line still takes the
     * ordinary downshift the speed asks for, one gear at a time.
     */
    Place(0, 5, 0);
    s_car.drive.motionState = CAR_MOTION_STANDING_START;
    ShiftPlayerGears(&s_car, 0);
    Check(s_car.drive.gear == 4, "a standing start only steps down one",
          s_car.drive.gear, 4);
    Check(s_car.drive.clutch == 0, "and does not touch the clutch",
          s_car.drive.clutch, 0);

    /* A shift already in progress is left alone, but the cooldown still runs
     * and stopping still drops the gear. */
    Place(0, 1, 1500);
    s_car.verticalMotionState = CAR_VERTICAL_RISING;
    ShiftPlayerGears(&s_car, 0);
    Check(s_car.drive.gear == 1, "no shift while one is in progress",
          s_car.drive.gear, 1);
}

int main(void) {
    BuildSpec();
    ManualTests();
    AutomaticTests();

    if (s_failures != 0) {
        printf("%d gearbox checks failed\n", s_failures);
        return 1;
    }
    printf("the gearbox picks the gear it always picked\n");
    return 0;
}
