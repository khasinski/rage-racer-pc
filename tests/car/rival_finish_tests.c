#include "game/rival.h"
#include <stdio.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    GameCarRuntime car = {.y = 100, .speed = 1000, .collisionFlag = 1,
        .bodyPitch = 10, .bodyYaw = 20, .bodyRoll = 30, .bodyRollVelocity = 5};
    GameCarRuntime other = car;
    FinishRival(&car, NULL, 3000, 0);
    CHECK(car.speed == 940 && car.wheelRotation == (3000 | 0x1000));
    CHECK(car.modelPitch == 10 && car.modelYaw == 20 && car.modelRoll == 30);
    CHECK(car.bodyRoll == 35 && car.modelY == 100);
    CHECK(other.speed == 1000 && other.bodyRoll == 30);
    GameCarRuntime restored = car;
    FinishRival(&car, NULL, 3000, 0);
    FinishRival(&restored, NULL, 3000, 0);
    CHECK(memcmp(&car, &restored, sizeof(car)) == 0);
    car.speed = 2;
    FinishRival(&car, NULL, 3000, 0);
    CHECK(car.speed == 0); /* Preserve the two separate integer divisions. */
    car.activeFlag = -1;
    restored = car;
    FinishRival(&car, NULL, 3000, 0);
    FinishRival(NULL, NULL, 3000, 0);
    CHECK(memcmp(&car, &restored, sizeof(car)) == 0);
    car = (GameCarRuntime){.y = 100, .verticalMotionState = CAR_VERTICAL_RISING,
        .verticalMotionRate = -20};
    FinishRival(&car, NULL, 3000, 1);
    CHECK(car.y == 80 && car.verticalMotionTimer == 1);
    CHECK(car.verticalMotionState == CAR_VERTICAL_RISING);
    return 0;
}
