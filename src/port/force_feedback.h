#ifndef RAGE_FORCE_FEEDBACK_H
#define RAGE_FORCE_FEEDBACK_H

/* Arcade torque for a wheel that has no tire model to sample.
 *
 * The retail car publishes steering, speed, heading error, grip loss and
 * impact kicks. This turns those into one constant-force sample. Centering
 * grows with speed and goes light in a slide. A collision is a lateral kick.
 * Damper and the end-stop are computed here so the device only has to play
 * one constant effect. Values are fractions of full scale, -1 to 1 for
 * torque and 0 to 1 for a gamepad's two rumble motors.
 *
 * The units below are the car's own: steerPos reaches lock at ±4096, speed
 * is weighted by 800, steeringLoadAngle tops out at a quarter turn (1024),
 * and the impact divisors bring a knockback of 64 or a body kick of 128 up
 * to full scale. */

typedef struct ForceFeedbackTuning {
    float gain;
    float center;
    float slide;
    float collision;
    float road;
    float damper;
    float minForce;
    float softLock;
    float engine;
    int invert;
    int enabled;
} ForceFeedbackTuning;

typedef struct ForceFeedbackSample {
    int steerPos;
    int speed;
    int steeringLoadAngle;
    int gripLossTimer;
    int collisionLateral;
    int roadImpulse;
    int crossSlope;
    float engineWave;
    float wheelAxis;
    float previousWheelAxis;
    float smoothed;
    int active;
} ForceFeedbackSample;

typedef struct ForceFeedbackOutput {
    float torque;
    float lowRumble;
    float highRumble;
    float smoothed;
} ForceFeedbackOutput;

ForceFeedbackTuning ForceFeedbackDefaultTuning(void);
ForceFeedbackOutput ForceFeedbackEvaluate(const ForceFeedbackTuning *tuning,
                                          const ForceFeedbackSample *sample);

#endif
