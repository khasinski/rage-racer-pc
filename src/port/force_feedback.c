#include "force_feedback.h"

#include <math.h>
#include <stddef.h>

enum {
    FFB_FULL_LOCK = 4096,
    FFB_FULL_SPEED = 800,
    FFB_FULL_SLIP = 1024,
    FFB_FULL_COLLISION = 64,
    FFB_FULL_ROAD = 128,
    FFB_FULL_CAMBER = 64
};

/* Peaks above three quarters of full scale ease toward the stop instead of
 * slamming into it. At or below that the sample is left alone, so a modest
 * centering force stays exactly what the scales asked for. */
static float SoftClip(float value) {
    float magnitude;
    float excess;

    if (!isfinite(value)) return 0.0f;
    magnitude = value < 0.0f ? -value : value;
    if (magnitude <= 0.75f) return value;
    if (magnitude > 1.5f) magnitude = 1.5f;
    excess = magnitude - 0.75f;
    magnitude = 0.75f + excess / (1.0f + 2.0f * excess);
    if (magnitude > 1.0f) magnitude = 1.0f;
    return value < 0.0f ? -magnitude : magnitude;
}

static float ClampSigned(float value) {
    if (!isfinite(value)) return 0.0f;
    if (value < -1.0f) return -1.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

static float ClampUnit(float value) {
    if (!isfinite(value)) return 0.0f;
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

static float DivideScale(int value, int scale) {
    return ClampSigned((float)value / (float)scale);
}

ForceFeedbackTuning ForceFeedbackDefaultTuning(void) {
    ForceFeedbackTuning tuning = {0};

    tuning.gain = 0.55f;
    tuning.center = 0.80f;
    tuning.slide = 0.70f;
    tuning.collision = 0.75f;
    tuning.road = 0.25f;
    tuning.damper = 0.20f;
    tuning.minForce = 0.06f;
    tuning.softLock = 0.85f;
    tuning.engine = 0.0f;
    tuning.invert = 0;
    tuning.enabled = 1;
    return tuning;
}

ForceFeedbackOutput ForceFeedbackEvaluate(const ForceFeedbackTuning *tuning,
                                          const ForceFeedbackSample *sample) {
    ForceFeedbackOutput output = {0};
    float steer, speed, slip, lighten, center, collision, road, damper, soft;
    float engine, target, gain, previous;
    int live;

    if (tuning == NULL || sample == NULL) return output;

    previous = isfinite(sample->smoothed) ? sample->smoothed : 0.0f;
    previous = ClampSigned(previous);
    live = tuning->enabled && sample->active;
    if (live) {
        steer = DivideScale(sample->steerPos, FFB_FULL_LOCK);
        speed = sample->speed <= 0
                    ? 0.0f
                    : ClampUnit((float)sample->speed / (float)FFB_FULL_SPEED);
        slip = ClampUnit(
            (sample->steeringLoadAngle < 0 ? -sample->steeringLoadAngle
                                           : sample->steeringLoadAngle) /
            (float)FFB_FULL_SLIP);
        lighten = 1.0f - slip * ClampUnit(tuning->slide);
        if (lighten < 0.0f) lighten = 0.0f;
        if (sample->gripLossTimer > 0) lighten *= 0.35f;
        center = -steer * speed * ClampUnit(tuning->center) * lighten;

        collision = DivideScale(sample->collisionLateral, FFB_FULL_COLLISION) *
                    ClampUnit(tuning->collision);
        road = (DivideScale(sample->roadImpulse, FFB_FULL_ROAD) * 0.65f +
                DivideScale(sample->crossSlope, FFB_FULL_CAMBER) * 0.35f) *
               ClampUnit(tuning->road);
        damper = -(ClampSigned(sample->wheelAxis) -
                   ClampSigned(sample->previousWheelAxis)) *
                 ClampUnit(tuning->damper);
        soft = 0.0f;
        if ((steer < 0.0f ? -steer : steer) > 0.90f) {
            float axis = ClampSigned(sample->wheelAxis);
            float past = (axis < 0.0f ? -axis : axis) - 0.98f;

            if (past > 0.0f) {
                if (past > 0.02f) past = 0.02f;
                soft = (axis > 0.0f ? -past : past) / 0.02f *
                       ClampUnit(tuning->softLock);
            }
        }
        engine = ClampSigned(sample->engineWave) * speed *
                 ClampUnit(tuning->engine);

        target = SoftClip(center + collision + road + damper + soft + engine);
        gain = ClampUnit(tuning->gain);
        target *= gain;
        if (tuning->invert) target = -target;
        {
            float minimum = ClampUnit(tuning->minForce);

            if (target > 0.02f && target < minimum) target = minimum;
            else if (target < -0.02f && target > -minimum) target = -minimum;
        }

        output.lowRumble = ClampUnit(
            (collision < 0.0f ? -collision : collision) * 0.85f +
            (road < 0.0f ? -road : road) * 0.50f);
        output.lowRumble *= gain;
        output.highRumble = ClampUnit(
            (sample->gripLossTimer > 0 ? 0.55f : 0.0f) + slip * 0.40f +
            (engine < 0.0f ? -engine : engine) * 0.25f);
        output.highRumble *= gain;
    } else {
        target = 0.0f;
    }

    output.smoothed = previous + (target - previous) * 0.45f;
    output.torque = output.smoothed;
    return output;
}
