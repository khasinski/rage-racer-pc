/*
 * Sweep every control path through UpdateCarBodyRoll. The function is called
 * once for each independent input state because its damping makes repeated
 * calls stateful. The digest protects the exact fixed-point behaviour while
 * the implementation is split into readable controller-specific helpers.
 */

#include "common.h"
#include "game/car.h"
#include "game/input_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"

#include <stdio.h>
#include <string.h>

void UpdateCarBodyRoll(PlayerCarRuntime *car);

u8 g_PadType;
volatile u16 g_PadHeld;
u16 g_PadButtonMapping[16];
s16 g_NegconSteer;

static u32 FoldWord(u32 digest, s32 value) {
    int byte;

    for (byte = 0; byte < 4; byte++) {
        digest ^= ((u32)value >> (byte * 8)) & 0xFF;
        digest *= 16777619U;
    }
    return digest;
}

int main(void) {
    static const s16 modes[] = {0, 2, 4};
    static const u8 padTypes[] = {0x41, 0x23, 0};
    static const s32 speeds[] = {0, 80, 81, 799, 800, 1600};
    static const s32 offsets[] = {-512, 0, 512};
    static const s32 steerPositions[] = {-5000, -4095, -1000, 0,
                                         1000, 4095, 5000};
    static const s32 steeringAngles[] = {-2000, 0, 2000};
    static const s32 rollVelocities[] = {-100, 0, 100};
    static const s16 negconPositions[] = {-128, 0, 127};
    static const u16 heldStates[] = {0, 1, 2, 3};
    static const u32 expected = 1168540033U;
    PlayerCarRuntime car;
    u32 digest = 2166136261U;
    int calls = 0;
    size_t mode, pad, autoSteer, backwards, speed, offset;
    size_t steer, angle, velocity, held, negcon;

    g_PadButtonMapping[0] = 1;
    g_PadButtonMapping[1] = 2;
    g_NegconMaxTwist = 0;

    for (mode = 0; mode < sizeof(modes) / sizeof(modes[0]); mode++)
    for (pad = 0; pad < sizeof(padTypes) / sizeof(padTypes[0]); pad++)
    for (autoSteer = 0; autoSteer < 2; autoSteer++)
    for (backwards = 0; backwards < 2; backwards++)
    for (speed = 0; speed < sizeof(speeds) / sizeof(speeds[0]); speed++)
    for (offset = 0; offset < sizeof(offsets) / sizeof(offsets[0]); offset++)
    for (steer = 0; steer < sizeof(steerPositions) / sizeof(steerPositions[0]); steer++)
    for (angle = 0; angle < sizeof(steeringAngles) / sizeof(steeringAngles[0]); angle++)
    for (velocity = 0; velocity < sizeof(rollVelocities) / sizeof(rollVelocities[0]); velocity++)
    for (held = 0; held < sizeof(heldStates) / sizeof(heldStates[0]); held++)
    for (negcon = 0; negcon < sizeof(negconPositions) / sizeof(negconPositions[0]); negcon++) {
        memset(&car, 0, sizeof(car));
        g_RacePhase = modes[mode];
        g_PadType = padTypes[pad];
        g_PlayerAutoSteer = (s16)autoSteer;
        g_PadHeld = heldStates[held];
        g_NegconSteer = negconPositions[negcon];
        car.facingBackwards = (s32)backwards;
        car.speed = speeds[speed];
        car.trackLateralOffset = offsets[offset];
        car.bodyYaw = 0x280;
        car.trackHeading.value = 0x140;
        car.drive.steerPos = steerPositions[steer];
        car.steeringAngle = steeringAngles[angle];
        car.bodyRollVelocity = rollVelocities[velocity];

        UpdateCarBodyRoll(&car);

        digest = FoldWord(digest, car.drive.steerPos);
        digest = FoldWord(digest, car.drive.trackCurveMode);
        digest = FoldWord(digest, car.steeringAngle);
        digest = FoldWord(digest, car.bodyRollVelocity);
        calls++;
    }

    if (digest != expected) {
        printf("FAIL: %d body-roll states digest to %u, expected %u\n",
               calls, digest, expected);
        return 1;
    }
    printf("all %d body-roll states preserved\n", calls);
    return 0;
}
