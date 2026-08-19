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
    Expect("rest", RageAxisCurve(0.0f, 0.0f, 1.0f, 0.0f, 1.0f), 0.0f);
    Expect("half", RageAxisCurve(0.5f, 0.0f, 1.0f, 0.0f, 1.0f), 0.5f);
    Expect("full", RageAxisCurve(1.0f, 0.0f, 1.0f, 0.0f, 1.0f), 1.0f);

    /* Deadzone eats the first slice of travel and rescales what is left. */
    Expect("inside deadzone", RageAxisCurve(0.1f, 0.2f, 1.0f, 0.0f, 1.0f), 0.0f);
    Expect("just past deadzone", RageAxisCurve(0.6f, 0.2f, 1.0f, 0.0f, 1.0f), 0.5f);

    /* Saturation reaches full deflection early. */
    Expect("saturated", RageAxisCurve(0.5f, 0.0f, 0.5f, 0.0f, 1.0f), 1.0f);

    /* Linearity is DuckStation's exponent: value raised to e^linearity. */
    Expect("linearity 0.5 at half", RageAxisCurve(0.5f, 0.0f, 1.0f, 0.5f, 1.0f),
           powf(0.5f, expf(0.5f)));
    Expect("linearity -0.5 at half", RageAxisCurve(0.5f, 0.0f, 1.0f, -0.5f, 1.0f),
           powf(0.5f, expf(-0.5f)));
    /* Positive linearity softens the middle, negative sharpens it. */
    if (!(RageAxisCurve(0.5f, 0.0f, 1.0f, 0.5f, 1.0f) < 0.5f)) {
        printf("positive linearity should sit below the linear response\n");
        failures++;
    }
    if (!(RageAxisCurve(0.5f, 0.0f, 1.0f, -0.5f, 1.0f) > 0.5f)) {
        printf("negative linearity should sit above the linear response\n");
        failures++;
    }
    /* The ends stay put whatever the curve does. */
    Expect("curved rest", RageAxisCurve(0.0f, 0.0f, 1.0f, 2.0f, 1.0f), 0.0f);
    Expect("curved full", RageAxisCurve(1.0f, 0.0f, 1.0f, 2.0f, 1.0f), 1.0f);

    /* Scaling multiplies afterwards and cannot push past full deflection. */
    Expect("scaled", RageAxisCurve(0.5f, 0.0f, 1.0f, 0.0f, 1.5f), 0.75f);
    Expect("scaling clamps", RageAxisCurve(0.9f, 0.0f, 1.0f, 0.0f, 4.0f), 1.0f);

    if (failures) {
        printf("%d axis curve assertion(s) failed\n", failures);
        return 1;
    }
    printf("axis curve matches the NeGcon response DuckStation applies\n");
    return 0;
}
