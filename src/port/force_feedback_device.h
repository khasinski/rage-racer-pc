#ifndef RAGE_FORCE_FEEDBACK_DEVICE_H
#define RAGE_FORCE_FEEDBACK_DEVICE_H

struct SDL_Joystick;

/* Drop a constant-force effect before the joystick it belongs to is closed. */
void ForceFeedbackDetachJoystick(struct SDL_Joystick *joystick);

/* The pause menu and the Esc panel share this choice. A harness that forces
 * the setting through the environment ignores the setters. */
int PortForceFeedbackEnabled(void);
void PortSetForceFeedbackEnabled(int enabled);
int PortForceFeedbackGainPercent(void);
void PortSetForceFeedbackGainPercent(int percent);

#endif
