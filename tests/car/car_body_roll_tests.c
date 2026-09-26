/*
 * Sweep every control path through UpdateCarSteering. The function is called
 * once for each independent input state because its damping makes repeated
 * calls stateful. The digest protects the exact fixed-point behaviour while
 * the input is separated from controller devices and race globals.
 */

#include "game/car_control.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

static u32 FoldWord(u32 digest, s32 value) {
    int byte;

    for (byte = 0; byte < 4; byte++) {
        digest ^= ((u32)value >> (byte * 8)) & 0xFF;
        digest *= 16777619U;
    }
    return digest;
}

static int CheckNeutralSteering(PlayerCarRuntime *car, const char *what) {
    if (car->drive.trackCurveMode != 0 || car->drive.steerPos != 0 ||
        car->steeringAngle != 0 || car->bodyRollVelocity != 0) {
        printf("FAIL: %s left steering state %d/%d/%d/%d\n", what,
               car->drive.trackCurveMode, car->drive.steerPos,
               car->steeringAngle, car->bodyRollVelocity);
        return 1;
    }
    return 0;
}

int main(void) {
    static const s16 modes[] = {0, 2, 4};
    static const SteeringMode controls[] = {STEERING_DIGITAL, STEERING_ANALOG, STEERING_CENTER};
    static const s32 speeds[] = {0, 80, 81, 799, 800, 1600};
    static const s32 offsets[] = {-512, 0, 512};
    static const s32 steerPositions[] = {-5000, -4095, -1000, 0,
                                         1000, 4095, 5000};
    static const s32 steeringAngles[] = {-2000, 0, 2000};
    static const s32 rollVelocities[] = {-100, 0, 100};
    static const s16 negconPositions[] = {-128, 0, 127};
    static const u16 heldStates[] = {0, 1, 2, 3};
    static const u32 expected = 2007581857U;
    PlayerCarRuntime car;
    u32 digest = 2166136261U;
    int calls = 0;
    size_t mode, pad, autoSteer, backwards, speed, offset;
    size_t steer, angle, velocity, held, negcon;

    SteeringInput input = {0};

    memset(&car, 0x55, sizeof(car));
    UpdateCarSteering(&car, &input);
    if (CheckNeutralSteering(&car, "pre-race phase") != 0) return 1;

    memset(&car, 0x55, sizeof(car));
    UpdateCarSteering(&car, &input);
    if (CheckNeutralSteering(&car, "unknown controller") != 0) return 1;

    for (mode = 0; mode < sizeof(modes) / sizeof(modes[0]); mode++)
    for (pad = 0; pad < sizeof(controls) / sizeof(controls[0]); pad++)
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
        input.mode = modes[mode] < 2 ? STEERING_CENTER
            : modes[mode] >= 4 || autoSteer ? STEERING_AUTOMATIC
            : controls[pad];
        input.left = (heldStates[held] & 1) != 0;
        input.right = (heldStates[held] & 2) != 0;
        input.angle = negconPositions[negcon] * (13 * 512) / 25;
        car.facingBackwards = (s32)backwards;
        car.speed = speeds[speed];
        car.trackLateralOffset = offsets[offset];
        car.bodyYaw = 0x280;
        car.trackHeading = 0x140;
        car.drive.steerPos = steerPositions[steer];
        car.steeringAngle = steeringAngles[angle];
        car.bodyRollVelocity = rollVelocities[velocity];

        UpdateCarSteering(&car, &input);

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


    memset(&car, 0, sizeof(car));
    input = (SteeringInput){.mode = STEERING_DIGITAL};
    car.speed = 800;
    car.bodyRollVelocity = INT_MAX;
    UpdateCarSteering(&car, &input);
    if (car.bodyRollVelocity != 268435455) {
        printf("FAIL: wrapped body-roll damping produced %d\n",
               car.bodyRollVelocity);
        return 1;
    }

    memset(&car, 0, sizeof(car));
    input = (SteeringInput){.mode = STEERING_DIGITAL, .left = 1};
    car.speed = 800;
    car.drive.steerPos = INT_MIN;
    UpdateCarSteering(&car, &input);
    if (car.steeringAngle != INT_MIN) {
        printf("FAIL: minimum digital steering produced %d\n",
               car.steeringAngle);
        return 1;
    }

    PlayerCarRuntime left = {0};
    PlayerCarRuntime right = {0};
    PlayerCarRuntime alone = {0};
    left.speed = right.speed = alone.speed = 800;
    SteeringInput leftInput = {.mode = STEERING_DIGITAL, .left = 1};
    SteeringInput rightInput = {.mode = STEERING_ANALOG, .angle = 2000};
    for (int i = 0; i < 20; i++) {
        UpdateCarSteering(&alone, &leftInput);
    }
    for (int i = 0; i < 20; i++) {
        UpdateCarSteering(&left, &leftInput);
        UpdateCarSteering(&right, &rightInput);
    }
    if (memcmp(&left, &alone, sizeof(left)) != 0 ||
        left.drive.trackCurveMode != 2 || right.drive.trackCurveMode != 1) {
        puts("FAIL independent digital and analog drivers");
        return 1;
    }
    PlayerCarRuntime restored = right;
    for (int i = 0; i < 10; i++) {
        UpdateCarSteering(&right, &rightInput);
        UpdateCarSteering(&left, &leftInput);
        UpdateCarSteering(&restored, &rightInput);
    }
    if (memcmp(&right, &restored, sizeof(right)) != 0) {
        puts("FAIL steering restored from car state");
        return 1;
    }
    printf("all %d body-roll states preserved; drivers are independent\n", calls);
    return 0;
}
