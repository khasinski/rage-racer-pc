#include <math.h>

enum {
    NEGCON_TWIST_CENTRE = 0x80,
    NEGCON_TWIST_MAX = 0x7F,
};


float AxisCurve(float value, float deadzone, float saturation,
                float linearity, float scaling) {
    if (!isfinite(value)) return 0.0f;
    if (!isfinite(deadzone)) deadzone = 0.0f;
    if (!isfinite(saturation)) saturation = 1.0f;
    if (!isfinite(linearity)) linearity = 0.0f;
    if (!isfinite(scaling)) scaling = 1.0f;
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

int NegconTwist(float shapedStick, int dpadLeft, int dpadRight, int range) {
    int deflection;
    int twist;
    if (!isfinite(shapedStick)) shapedStick = 0.0f;
    if (shapedStick < -1.0f) shapedStick = -1.0f;
    if (shapedStick > 1.0f) shapedStick = 1.0f;
    deflection = (int)(shapedStick * (float)NEGCON_TWIST_MAX);
    if (range <= 0) range = NEGCON_TWIST_MAX;
    if (range > NEGCON_TWIST_CENTRE) range = NEGCON_TWIST_CENTRE;
    if (dpadLeft && deflection > -range) deflection = -range;
    if (dpadRight && deflection < range) deflection = range;
    twist = NEGCON_TWIST_CENTRE + deflection;
    if (twist < 0) twist = 0;
    if (twist > 0xFF) twist = 0xFF;
    return twist;
}

float JoystickPedalAxis(int value, int inverted) {
    float result;
    if (value < -32768) value = -32768;
    if (value > 32767) value = 32767;
    result = ((float)value + 32768.0f) / 65535.0f;
    return inverted ? 1.0f - result : result;
}
