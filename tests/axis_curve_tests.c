#include <math.h>
#include <stdio.h>

#include "../src/port/axis_curve.h"

static int failures;

static void Expect(const char *what, float got, float want) {
    if (fabsf(got - want) > 0.002f) {
        printf("%s: expected %.4f, got %.4f\n", what, want, got);
        failures++;
    }
}

int main(void) {
    /* Defaults leave the axis alone. */
    Expect("rest", AxisCurve(0.0f, 0.0f, 1.0f, 0.0f, 1.0f), 0.0f);
    Expect("half", AxisCurve(0.5f, 0.0f, 1.0f, 0.0f, 1.0f), 0.5f);
    Expect("full", AxisCurve(1.0f, 0.0f, 1.0f, 0.0f, 1.0f), 1.0f);

    /* Deadzone eats the first slice of travel and rescales what is left. */
    Expect("inside deadzone", AxisCurve(0.1f, 0.2f, 1.0f, 0.0f, 1.0f), 0.0f);
    Expect("just past deadzone", AxisCurve(0.6f, 0.2f, 1.0f, 0.0f, 1.0f), 0.5f);

    /* Saturation reaches full deflection early. */
    Expect("saturated", AxisCurve(0.5f, 0.0f, 0.5f, 0.0f, 1.0f), 1.0f);

    /* Linearity is DuckStation's exponent: value raised to e^linearity. */
    Expect("linearity 0.5 at half", AxisCurve(0.5f, 0.0f, 1.0f, 0.5f, 1.0f),
           powf(0.5f, expf(0.5f)));
    Expect("linearity -0.5 at half", AxisCurve(0.5f, 0.0f, 1.0f, -0.5f, 1.0f),
           powf(0.5f, expf(-0.5f)));
    /* Positive linearity softens the middle, negative sharpens it. */
    if (!(AxisCurve(0.5f, 0.0f, 1.0f, 0.5f, 1.0f) < 0.5f)) {
        printf("positive linearity should sit below the linear response\n");
        failures++;
    }
    if (!(AxisCurve(0.5f, 0.0f, 1.0f, -0.5f, 1.0f) > 0.5f)) {
        printf("negative linearity should sit above the linear response\n");
        failures++;
    }
    /* The ends stay put whatever the curve does. */
    Expect("curved rest", AxisCurve(0.0f, 0.0f, 1.0f, 2.0f, 1.0f), 0.0f);
    Expect("curved full", AxisCurve(1.0f, 0.0f, 1.0f, 2.0f, 1.0f), 1.0f);

    /* Scaling multiplies afterwards and cannot push past full deflection. */
    Expect("scaled", AxisCurve(0.5f, 0.0f, 1.0f, 0.0f, 1.5f), 0.75f);
    Expect("scaling clamps", AxisCurve(0.9f, 0.0f, 1.0f, 0.0f, 4.0f), 1.0f);

    /* Twist reporting: the stick spans the byte whatever the calibration says,
     * so the smallest twist range still reaches full lock once the game
     * subtracts its play. Scaling here as well was what broke calibrating. */
    Expect("stick centred", (float)NegconTwist(0.0f, 0, 0, 25), 128.0f);
    Expect("stick full right, small range",
           (float)NegconTwist(1.0f, 0, 0, 25), 255.0f);
    Expect("stick full left, small range",
           (float)NegconTwist(-1.0f, 0, 0, 25), 1.0f);
    Expect("stick full right, large range",
           (float)NegconTwist(1.0f, 0, 0, 113), 255.0f);

    /* A d-pad press reproduces retail's synthetic twist, which is the range. */
    Expect("d-pad right at range 25", (float)NegconTwist(0.0f, 0, 1, 25), 153.0f);
    Expect("d-pad left at range 113", (float)NegconTwist(0.0f, 1, 0, 113), 15.0f);

    /* Whichever input is pushed further wins, in both directions. */
    Expect("stick beats d-pad", (float)NegconTwist(1.0f, 0, 1, 25), 255.0f);
    Expect("d-pad beats a nudged stick",
           (float)NegconTwist(0.05f, 0, 1, 25), 153.0f);
    Expect("opposing d-pad still steers from a centred stick",
           (float)NegconTwist(0.0f, 1, 0, 25), 103.0f);

    Expect("inverted pedal idle", JoystickPedalAxis(32767, 1), 0.0f);
    Expect("inverted pedal half", JoystickPedalAxis(0, 1), 0.5f);
    Expect("inverted pedal pressed", JoystickPedalAxis(-32768, 1), 1.0f);
    Expect("normal pedal idle", JoystickPedalAxis(-32768, 0), 0.0f);
    Expect("normal pedal pressed", JoystickPedalAxis(32767, 0), 1.0f);

    if (failures) {
        printf("%d axis curve assertion(s) failed\n", failures);
        return 1;
    }
    printf("axis curve matches the NeGcon response DuckStation applies\n");
    return 0;
}
