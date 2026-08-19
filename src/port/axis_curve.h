#ifndef RAGE_AXIS_CURVE_H
#define RAGE_AXIS_CURVE_H

/* Response shaping for an analog axis, matching what DuckStation applies to a
 * NeGcon so a value copied from there behaves the same here.
 *
 * `value` is the axis magnitude, 0 at rest and 1 at full deflection. Deadzone
 * and saturation are fractions of that travel, linearity is the exponent
 * DuckStation spells as e^linearity, and scaling multiplies the result.
 * Deadzone 0, saturation 1, linearity 0 and scaling 1 leave the axis alone.
 */
float RageAxisCurve(float value, float deadzone, float saturation,
                    float linearity, float scaling);

#endif
