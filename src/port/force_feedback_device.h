#ifndef RAGE_FORCE_FEEDBACK_DEVICE_H
#define RAGE_FORCE_FEEDBACK_DEVICE_H

struct SDL_Joystick;

/* Drop a constant-force effect before the joystick it belongs to is closed. */
void ForceFeedbackDetachJoystick(struct SDL_Joystick *joystick);

#endif
