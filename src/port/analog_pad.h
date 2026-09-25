#ifndef RAGE_ANALOG_PAD_H
#define RAGE_ANALOG_PAD_H

struct SDL_Joystick;
struct SDL_Gamepad;

/* The shaped steering axis from the last sample, -1 to 1. Zero when no analog
 * device is driving. */
float AnalogSteeringDeflection(void);
int AnalogWheelActive(void);
struct SDL_Joystick *AnalogWheelJoystick(void);
struct SDL_Gamepad *AnalogActiveGamepad(void);

#endif
