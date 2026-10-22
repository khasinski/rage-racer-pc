#ifndef RAGE_SESSION_OPTIONS_H
#define RAGE_SESSION_OPTIONS_H

/* Live copies of the settings the Esc panel can change. Graphics stay on
 * RagePortConfig. These take effect immediately. Analog, the wheel, steering
 * feel, the HUD and the chase camera last until the game closes. Music and
 * effects are the save-game volumes. Force feedback is the remembered player
 * choice the pause menu already uses. */

typedef struct PortSessionOptions {
    int analog;
    int wheel;
    float steeringLinearity;
    int forceFeedback;
    int forceFeedbackGain;
    int music;
    int effects;
    int hudEdges;
    int showLapTimes;
    int showTimeLimit;
    float chaseDistance;
    float chaseHeight;
    float chaseLookahead;
} PortSessionOptions;

void PortSessionOptionsRead(PortSessionOptions *options);
void PortSessionOptionsApply(const PortSessionOptions *options);

#endif
