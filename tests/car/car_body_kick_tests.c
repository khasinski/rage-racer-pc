#include "game/car.h"
#include "game/car_motion_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

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
    s32 trackAngle = 0;
    s32 random = 0;

    memset(&car, 0, sizeof(car));
    car.verticalMotionTimer = 7;
    BeginCarBodyKick(&car, CAR_BODY_KICK_LANDING, trackAngle, random);
    CHECK(car.motionMode == CAR_BODY_KICK_LANDING &&
          car.motionModeTimer == 30);
    CHECK(car.motionValue == 56);

    memset(&car, 0, sizeof(car));
    car.verticalMotionTimer = -1;
    BeginCarBodyKick(&car, CAR_BODY_KICK_LANDING, trackAngle, random);
    CHECK(car.motionValue == -8);

    car.verticalMotionTimer = INT16_MAX;
    BeginCarBodyKick(&car, CAR_BODY_KICK_LANDING, trackAngle, random);
    CHECK(car.motionValue == -8);

    memset(&car, 0, sizeof(car));
    car.speed = 0x140 + 0x1000;
    car.bodyYaw = 0x400;
    trackAngle = 0;
    random = 0;
    BeginCarBodyKick(&car, CAR_BODY_KICK_CORNERING, trackAngle, random);
    CHECK(car.motionModeTimer == 30 && car.motionValue == 0x400);

    random = 0x80;
    BeginCarBodyKick(&car, CAR_BODY_KICK_CORNERING, trackAngle, random);
    CHECK(car.motionValue == -0x400);

    car.speed = 0x13F;
    BeginCarBodyKick(&car, CAR_BODY_KICK_CORNERING, trackAngle, random);
    CHECK(car.motionValue == 0);

    car.speed = INT_MAX;
    random = 0;
    BeginCarBodyKick(&car, CAR_BODY_KICK_CORNERING, trackAngle, random);
    CHECK(car.motionValue == -80);

    car.speed = INT_MIN;
    BeginCarBodyKick(&car, CAR_BODY_KICK_CORNERING, trackAngle, random);
    CHECK(car.motionValue == -80);

    car.motionModeTimer = 12;
    car.motionValue = 34;
    car.motionMode = CAR_BODY_KICK_CORNERING;
    BeginCarBodyKick(&car, (CarBodyKickMode)7, trackAngle, random);
    CHECK(car.motionMode == CAR_BODY_KICK_CORNERING);
    CHECK(car.motionModeTimer == 12 && car.motionValue == 34);

    GameCarRuntime first = {0};
    GameCarRuntime second = {0};
    first.speed = second.speed = 0x140 + 0x1000;
    first.bodyYaw = second.bodyYaw = 0x400;
    BeginCarBodyKick(&first, CAR_BODY_KICK_CORNERING, 0, 0);
    BeginCarBodyKick(&second, CAR_BODY_KICK_CORNERING, 0, 0x80);
    CHECK(first.motionValue == 0x400 && second.motionValue == -0x400);
    GameCarRuntime saved = first;
    UpdateCarBodyKick(&first);
    UpdateCarBodyKick(&second);
    CHECK(first.bodyRoll == -second.bodyRoll);
    GameCarRuntime restored = saved;
    UpdateCarBodyKick(&restored);
    CHECK(memcmp(&first, &restored, sizeof(first)) == 0);
    for (int frame = 1; frame < CAR_BODY_KICK_DURATION; frame++) {
        UpdateCarBodyKick(&first);
        UpdateCarBodyKick(&second);
    }
    CHECK(first.motionMode == CAR_BODY_KICK_INACTIVE && first.motionModeTimer == 0);
    CHECK(second.motionMode == CAR_BODY_KICK_INACTIVE && second.motionModeTimer == 0);
    CHECK(first.bodyKickOffset == 0 && second.bodyKickOffset == 0);
    saved = first;
    UpdateCarBodyKick(&first);
    CHECK(memcmp(&saved, &first, sizeof(first)) == 0);
    puts("car body kick tests passed");

    return 0;
}
