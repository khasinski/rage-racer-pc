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
