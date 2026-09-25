#include "force_feedback.h"

#include <math.h>
#include <stdio.h>

static int s_failures;

static void Expect(const char *what, float got, float want) {
    if (!(fabsf(got - want) <= 0.002f)) {
        printf("%s: expected %.4f, got %.4f\n", what, want, got);
        s_failures++;
    }
}

static ForceFeedbackTuning Scales(float gain, float center, float slide,
                                  float collision, float road, float damper,
                                  float minForce, float softLock,
                                  float engine) {
    ForceFeedbackTuning tuning = {0};

    tuning.gain = gain;
    tuning.center = center;
    tuning.slide = slide;
    tuning.collision = collision;
    tuning.road = road;
    tuning.damper = damper;
    tuning.minForce = minForce;
    tuning.softLock = softLock;
    tuning.engine = engine;
    tuning.enabled = 1;
    return tuning;
}

static ForceFeedbackSample Held(float smoothed) {
    ForceFeedbackSample sample = {0};

    sample.smoothed = smoothed;
    sample.active = 1;
    return sample;
}

int main(void) {
    ForceFeedbackTuning defaults = ForceFeedbackDefaultTuning();
    ForceFeedbackTuning tuning;
    ForceFeedbackSample sample;
    ForceFeedbackOutput output;

    Expect("default gain", defaults.gain, 0.55f);
    Expect("default center", defaults.center, 0.80f);
    Expect("default slide", defaults.slide, 0.70f);
    Expect("default collision", defaults.collision, 0.75f);
    Expect("default road", defaults.road, 0.25f);
    Expect("default damper", defaults.damper, 0.20f);
    Expect("default minimum", defaults.minForce, 0.06f);
    Expect("default soft lock", defaults.softLock, 0.85f);
    Expect("default engine", defaults.engine, 0.0f);
    if (!defaults.enabled || defaults.invert) {
        printf("defaults should be enabled and uninverted\n");
        s_failures++;
    }

    /* Half lock at full speed is a centering force of exactly half. Passing
     * that value back in as the previous sample holds the low-pass still. */
    tuning = Scales(1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    sample = Held(-0.5f);
    sample.steerPos = 2048;
    sample.speed = 800;
    output = ForceFeedbackEvaluate(&tuning, &sample);
    Expect("half lock centers", output.torque, -0.5f);

    sample = Held(-0.5f);
    sample.steerPos = 4096;
    sample.speed = 400;
    output = ForceFeedbackEvaluate(&tuning, &sample);
    Expect("half speed centers", output.torque, -0.5f);

    sample = Held(0.0f);
    sample.steerPos = -4096;
    sample.speed = 0;
    output = ForceFeedbackEvaluate(&tuning, &sample);
    Expect("standstill has no centering", output.torque, 0.0f);

    tuning.slide = 1.0f;
    sample = Held(0.0f);
    sample.steerPos = 4096;
    sample.speed = 800;
    sample.steeringLoadAngle = 1024;
    output = ForceFeedbackEvaluate(&tuning, &sample);
    Expect("full slide removes centering", output.torque, 0.0f);

    tuning.slide = 0.0f;
    sample = Held(-0.175f);
    sample.steerPos = 2048;
    sample.speed = 800;
    sample.gripLossTimer = 8;
    output = ForceFeedbackEvaluate(&tuning, &sample);
    Expect("grip loss keeps a third of the centering", output.torque, -0.175f);

    tuning = Scales(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    sample = Held(0.5f);
    sample.collisionLateral = 32;
    output = ForceFeedbackEvaluate(&tuning, &sample);
    Expect("collision follows the lateral kick", output.torque, 0.5f);

    sample = Held(-0.5f);
    sample.collisionLateral = -32;
    output = ForceFeedbackEvaluate(&tuning, &sample);
    Expect("opposite collision kicks the other way", output.torque, -0.5f);

    tuning = Scales(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    sample = Held(-0.5f);
    sample.wheelAxis = 0.5f;
    output = ForceFeedbackEvaluate(&tuning, &sample);
    Expect("damper opposes a fast hand", output.torque, -0.5f);

    tuning = Scales(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f);
    sample = Held(0.0f);
    sample.steerPos = 4096;
    sample.wheelAxis = 0.5f;
    output = ForceFeedbackEvaluate(&tuning, &sample);
    Expect("soft lock waits until the rim is past the game", output.torque,
           0.0f);
    sample = Held(-0.5f);
    sample.steerPos = 4096;
    sample.wheelAxis = 1.0f;
    output = ForceFeedbackEvaluate(&tuning, &sample);
    Expect("soft lock pushes back at full lock", output.torque, -0.5f);

    tuning = Scales(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    sample = Held(0.5f);
    sample.speed = 800;
    sample.engineWave = 0.5f;
    output = ForceFeedbackEvaluate(&tuning, &sample);
    Expect("engine follows its wave", output.torque, 0.5f);
    tuning.engine = 0.0f;
    sample.engineWave = 1.0f;
    sample.smoothed = 0.0f;
    output = ForceFeedbackEvaluate(&tuning, &sample);
    Expect("engine stays silent at its default scale", output.torque, 0.0f);

    tuning = Scales(1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.06f, 0.0f, 0.0f);
    sample = Held(-0.06f);
    sample.steerPos = 4096;
    sample.speed = 800;
    tuning.center = 0.03f;
    output = ForceFeedbackEvaluate(&tuning, &sample);
    Expect("minimum force lifts a weak torque", output.torque, -0.06f);
    tuning.center = 0.01f;
    sample.smoothed = -0.01f;
    output = ForceFeedbackEvaluate(&tuning, &sample);
    Expect("minimum force leaves a dead band alone", output.torque, -0.01f);

    tuning = Scales(1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    tuning.invert = 1;
    sample = Held(0.5f);
    sample.steerPos = 2048;
    sample.speed = 800;
    output = ForceFeedbackEvaluate(&tuning, &sample);
    Expect("invert flips the wheel", output.torque, 0.5f);
    Expect("invert leaves rumble unsigned", output.lowRumble, 0.0f);

    tuning.invert = 0;
    tuning.gain = 0.0f;
    sample = Held(0.0f);
    sample.steerPos = 4096;
    sample.speed = 800;
    output = ForceFeedbackEvaluate(&tuning, &sample);
    Expect("zero gain is silence", output.torque, 0.0f);

    tuning = Scales(1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    sample = Held(0.0f);
    sample.steerPos = 2048;
    sample.speed = 800;
    output = ForceFeedbackEvaluate(&tuning, &sample);
    Expect("low-pass covers part of a new force", output.torque, -0.225f);

    sample.active = 0;
    sample.smoothed = 1.0f;
    sample.steerPos = 4096;
    sample.speed = 800;
    output = ForceFeedbackEvaluate(&tuning, &sample);
    Expect("paused race releases toward center", output.torque, 0.55f);
    Expect("paused race does not rumble", output.lowRumble, 0.0f);
    Expect("paused race does not buzz", output.highRumble, 0.0f);

    tuning.enabled = 0;
    sample = Held(0.0f);
    sample.steerPos = 4096;
    sample.speed = 800;
    sample.collisionLateral = 64;
    sample.gripLossTimer = 4;
    output = ForceFeedbackEvaluate(&tuning, &sample);
    Expect("disabled feedback is zero", output.torque, 0.0f);
    Expect("disabled feedback does not rumble", output.highRumble, 0.0f);

    tuning = Scales(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    sample = Held(0.0f);
    sample.collisionLateral = 64;
    sample.gripLossTimer = 1;
    output = ForceFeedbackEvaluate(&tuning, &sample);
    if (!(output.lowRumble > 0.8f && output.highRumble > 0.5f)) {
        printf("impact should reach both rumble motors (low %.3f high %.3f)\n",
               output.lowRumble, output.highRumble);
        s_failures++;
    }

    if (s_failures != 0) {
        printf("%d force feedback checks failed\n", s_failures);
        return 1;
    }
    puts("force feedback torque matches the arcade scales");
    return 0;
}
