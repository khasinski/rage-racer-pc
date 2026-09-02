#include "game/car.h"

#include <stdio.h>
#include <string.h>

static s32 s_trackAngle;
static s32 s_random;

s32 InterpolateTrackAngle(s32 pointIndex, s32 weight) {
    (void)pointIndex;
    (void)weight;
    return s_trackAngle;
}

s32 Random15(void) {
    return s_random;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    GameCarRuntime car;

    memset(&car, 0, sizeof(car));
    car.verticalMotionTimer = 7;
    StartCarBodyKick(&car, CAR_BODY_KICK_LANDING);
    CHECK(car.motionMode == CAR_BODY_KICK_LANDING &&
          car.motionModeTimer == 30);
    CHECK(car.motionValue.value == 56);

    memset(&car, 0, sizeof(car));
    car.verticalMotionTimer = -1;
    StartCarBodyKick(&car, CAR_BODY_KICK_LANDING);
    CHECK(car.motionValue.value == -8);

    memset(&car, 0, sizeof(car));
    car.speed = 0x140 + 0x1000;
    car.bodyYaw = 0x400;
    s_trackAngle = 0;
    s_random = 0;
    StartCarBodyKick(&car, CAR_BODY_KICK_CORNERING);
    CHECK(car.motionModeTimer == 30 && car.motionValue.value == 0x400);

    s_random = 0x80;
    StartCarBodyKick(&car, CAR_BODY_KICK_CORNERING);
    CHECK(car.motionValue.value == -0x400);

    car.speed = 0x13F;
    StartCarBodyKick(&car, CAR_BODY_KICK_CORNERING);
    CHECK(car.motionValue.value == 0);

    car.motionModeTimer = 12;
    car.motionValue.value = 34;
    car.motionMode = CAR_BODY_KICK_CORNERING;
    StartCarBodyKick(&car, 7);
    CHECK(car.motionMode == CAR_BODY_KICK_CORNERING);
    CHECK(car.motionModeTimer == 12 && car.motionValue.value == 34);

    puts("car body kick tests passed");
    return 0;
}
