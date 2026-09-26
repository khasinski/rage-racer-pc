#include "game/car_shift.h"

#include <stdio.h>
#include <string.h>

static unsigned s_buttons;

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

/* A six-speed box that shifts up at 1000 internal speed units and down at 800. */
static void BuildSpec(void) {
    int i;

    memset(&s_spec, 0, sizeof(s_spec));
    s_spec.topGear = 6;
    for (i = 0; i < 6; i++) {
        s_spec.shiftPoints[i].upshiftSpeed = (s16)(1000 * (i + 1));
        s_spec.shiftPoints[i].downshiftSpeed = (s16)(800 * i);
    }
}

static void Place(int manual, s32 gear, s32 speed) {
    memset(&s_car, 0, sizeof(s_car));
    s_car.drive.manual = (s16)manual;
    s_car.drive.gear = (s16)gear;
    s_car.speed = speed;
    s_car.drive.clutch = 0;
    s_car.verticalMotionState = CAR_VERTICAL_GROUNDED;
    s_car.drive.motionState = CAR_MOTION_DRIVING;
    s_buttons = 0;
    s_car.drive.autoShiftCooldown = 0;
    s_car.drive.steerHoldFrames = 7;
}

static void Step(void) {
    ShiftCarGears(&s_car, &s_spec, (s_buttons & SHIFT_UP) != 0,
                  (s_buttons & SHIFT_DOWN) != 0);
}

static void ManualTests(void) {
    /* The buttons move one gear each. */
    Place(1, 3, 2000);
    s_buttons = SHIFT_UP;
    Step();
    Check(s_car.drive.gear == 4, "manual up", s_car.drive.gear, 4);
    Check(s_car.drive.steerHoldFrames == 0, "shifting clears the steering hold",
          s_car.drive.steerHoldFrames, 0);

    Place(1, 3, 2000);
    s_buttons = SHIFT_DOWN;
    Step();
    Check(s_car.drive.gear == 2, "manual down", s_car.drive.gear, 2);

    Place(1, 3, 2000);
    s_buttons = SHIFT_UP | SHIFT_DOWN;
    Step();
    Check(s_car.drive.gear == 3, "simultaneous shifts cancel in order",
          s_car.drive.gear, 3);

    /* Neither goes past its end of the box. */
    Place(1, 6, 2000);
    s_buttons = SHIFT_UP;
    Step();
    Check(s_car.drive.gear == 6, "no gear above the top", s_car.drive.gear, 6);

    Place(1, 1, 2000);
    s_buttons = SHIFT_DOWN;
    Step();
    Check(s_car.drive.gear == 1, "no gear below first", s_car.drive.gear, 1);

    /* Mid-shift the box will not take another one. */
    Place(1, 3, 2000);
    s_car.drive.clutch = 5;
    s_buttons = SHIFT_UP;
    Step();
    Check(s_car.drive.gear == 3, "no upshift while the clutch is out",
          s_car.drive.gear, 3);

    /* Downshifting is allowed mid-shift, which is retail's asymmetry, not a
     * slip here: only the upshift checks the clutch. */
    Place(1, 3, 2000);
    s_car.drive.clutch = 5;
    s_buttons = SHIFT_DOWN;
    Step();
    Check(s_car.drive.gear == 2, "downshift ignores the clutch",
          s_car.drive.gear, 2);

    Place(1, 0, 0);
    Step();
    Check(s_car.drive.gear == 1, "invalid low gear is repaired",
          s_car.drive.gear, 1);

    Place(1, 7, 0);
    Step();
    Check(s_car.drive.gear == 6, "invalid high gear is repaired",
          s_car.drive.gear, 6);

    s_spec.topGear = 3;
    Place(1, 5, 0);
    Step();
    Check(s_car.drive.gear == 3, "gear is capped by the configured gearbox",
          s_car.drive.gear, 3);

    s_spec.topGear = 0;
    Place(1, 3, 0);
    Step();
    Check(s_car.drive.gear == 1, "invalid low top gear falls back to first",
          s_car.drive.gear, 1);

    s_spec.topGear = 7;
    Place(1, 7, 0);
    Step();
    Check(s_car.drive.gear == 6, "invalid high top gear falls back to sixth",
          s_car.drive.gear, 6);
    s_spec.topGear = CAR_FORWARD_GEAR_COUNT;
}

static void AutomaticTests(void) {
    Place(0, 0, 500);
    Step();
    Check(s_car.drive.gear == 1, "auto repairs gear before table lookup",
          s_car.drive.gear, 1);

    /* Above the current gear's upshift speed it takes the next one. */
    Place(0, 1, 1500);
    Step();
    Check(s_car.drive.gear == 2, "auto up", s_car.drive.gear, 2);
    Check(s_car.drive.autoShiftCooldown == 25 - 1, "and starts the cooldown",
          s_car.drive.autoShiftCooldown, 24);

    /* Below the current gear's downshift speed it drops one. */
    Place(0, 4, 2000);
    Step();
    Check(s_car.drive.gear == 3, "auto down", s_car.drive.gear, 3);

    /* Between the two it stays where it is. */
    Place(0, 3, 2500);
    Step();
    Check(s_car.drive.gear == 3, "auto holds between the two speeds",
          s_car.drive.gear, 3);

    /* Top gear has nothing above it however fast the car goes. */
    Place(0, 6, 100000);
    Step();
    Check(s_car.drive.gear == 6, "auto stops at the top gear",
          s_car.drive.gear, 6);

    /* The cooldown blocks both directions while it runs. */
    Place(0, 1, 1500);
    s_car.drive.autoShiftCooldown = 5;
    Step();
    Check(s_car.drive.gear == 1, "the cooldown blocks an upshift",
          s_car.drive.gear, 1);
    Check(s_car.drive.autoShiftCooldown == 4, "and counts down", s_car.drive.autoShiftCooldown, 4);

    Place(0, 4, 2000);
    s_car.drive.autoShiftCooldown = 5;
    Step();
    Check(s_car.drive.gear == 4, "the cooldown blocks a downshift",
          s_car.drive.gear, 4);

    /* Braking hard runs it out twice as fast, so a car slowing down gets its
     * gears sooner. */
    Place(0, 4, 2000);
    s_car.drive.autoShiftCooldown = 10;
    s_car.drive.brakeInput = 129;
    Step();
    Check(s_car.drive.autoShiftCooldown == 8, "braking halves the wait",
          s_car.drive.autoShiftCooldown, 8);

    Place(0, 4, 2000);
    s_car.drive.autoShiftCooldown = 10;
    s_car.drive.brakeInput = 128;
    Step();
    Check(s_car.drive.autoShiftCooldown == 9, "just short of hard braking does not",
          s_car.drive.autoShiftCooldown, 9);

    /* Coming to a stop drops straight to first and frees the clutch. */
    Place(0, 5, 0);
    s_car.drive.clutch = 4;
    s_car.drive.autoShiftCooldown = 10;
    Step();
    Check(s_car.drive.gear == 1, "stopping drops to first", s_car.drive.gear,
          1);
    Check(s_car.drive.clutch == 0, "and lets the clutch in",
          s_car.drive.clutch, 0);
    Check(s_car.drive.autoShiftCooldown == 0, "and clears the wait", s_car.drive.autoShiftCooldown,
          0);

    /*
     * Except on the grid. The guard is narrower than it looks: it only stops
     * the drop straight to first, so a car sitting on the line still takes the
     * ordinary downshift the speed asks for, one gear at a time.
     */
    Place(0, 5, 0);
    s_car.drive.motionState = CAR_MOTION_STANDING_START;
    Step();
    Check(s_car.drive.gear == 4, "a standing start only steps down one",
          s_car.drive.gear, 4);
    Check(s_car.drive.clutch == 0, "and does not touch the clutch",
          s_car.drive.clutch, 0);

    /* A shift already in progress is left alone, but the cooldown still runs
     * and stopping still drops the gear. */
    Place(0, 1, 1500);
    s_car.verticalMotionState = CAR_VERTICAL_RISING;
    Step();
    Check(s_car.drive.gear == 1, "no shift while one is in progress",
          s_car.drive.gear, 1);
}

int main(void) {
    BuildSpec();
    ManualTests();
    AutomaticTests();

    /* Stepping another driver's gearbox cannot consume this driver's wait,
     * steering hold or shift table. Restoring the car restores its timers. */
    Place(0, 1, 1500);
    PlayerCarRuntime first = s_car;
    PlayerCarRuntime second = s_car;
    GameCarSpec secondSpec = s_spec;
    secondSpec.shiftPoints[0].upshiftSpeed = 2000;
    ShiftCarGears(&first, &s_spec, 0, 0);
    PlayerCarRuntime saved = first;
    for (int i = 0; i < 30; i++) {
        ShiftCarGears(&second, &secondSpec, 0, 0);
    }
    Check(first.drive.autoShiftCooldown == 24, "other car leaves cooldown alone",
          first.drive.autoShiftCooldown, 24);
    Check(second.drive.gear == 1, "other car uses its own shift table",
          second.drive.gear, 1);
    Check(second.drive.steerHoldFrames == 7, "other car keeps steering hold",
          second.drive.steerHoldFrames, 7);
    ShiftCarGears(&first, &s_spec, 0, 0);
    ShiftCarGears(&saved, &s_spec, 0, 0);
    Check(memcmp(&first, &saved, sizeof(first)) == 0,
          "restored gearbox continues identically", 0, 0);

    if (s_failures != 0) {
        printf("%d gearbox checks failed\n", s_failures);
        return 1;
    }
    printf("the gearbox picks the gear it always picked\n");
    return 0;
}
