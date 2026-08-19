#include <math.h>

#include "axis_curve.h"

float RageAxisCurve(float value, float deadzone, float saturation,
                    float linearity, float scaling) {
    if (saturation <= deadzone) saturation = deadzone + 0.0001f;
    value = (value - deadzone) / (saturation - deadzone);
    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;
    /* DuckStation raises the travelled fraction to e^linearity, so 0 is linear,
     * positive softens the middle of the throw and negative sharpens it. */
    value = powf(value, expf(linearity));
    value *= scaling;
    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;
    return value;
}

#define NEGCON_TWIST_CENTRE 0x80
#define NEGCON_TWIST_MAX 0x7F

int RageNegconTwist(float shapedStick, int dpadLeft, int dpadRight, int range) {
    int deflection = (int)(shapedStick * (float)NEGCON_TWIST_MAX);
    int twist;
    if (range <= 0) range = NEGCON_TWIST_MAX;
    if (dpadLeft && deflection > -range) deflection = -range;
    if (dpadRight && deflection < range) deflection = range;
    twist = NEGCON_TWIST_CENTRE + deflection;
    if (twist < 0) twist = 0;
    if (twist > 0xFF) twist = 0xFF;
    return twist;
}
