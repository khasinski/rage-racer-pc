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


/* Combine a shaped stick reading with the d-pad into the twist byte a NeGcon
 * reports. The stick spans the whole byte, as the hardware does whatever the
 * calibration says, while a d-pad press reproduces retail's own synthetic
 * twist, which is exactly the configured range. Whichever is pushed further
 * wins, so either input steers and neither cancels the other. */
int RageNegconTwist(float shapedStick, int dpadLeft, int dpadRight, int range);

/* Normalize a raw SDL-style pedal axis. Many wheels report +32767 at rest and
 * -32768 when fully pressed, while others use the opposite orientation. */
float RageJoystickPedalAxis(int value, int inverted);

#endif
