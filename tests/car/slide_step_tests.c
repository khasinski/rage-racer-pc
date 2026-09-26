#include "game/car_motion_internal.h"
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

int main(void) {
    GameCarRuntime first = {.speed = 960};
    StepCarSlide(&first, 100, 0);
    CHECK(first.slideInput == 0 && first.yawRate == 0);
    first.speed = 1600;
    GameCarRuntime second = first;
    StepCarSlide(&first, 100, 0);
    StepCarSlide(&second, 100, 1);
    CHECK(first.slideInput == 200 && second.slideInput == -200);
    GameCarRuntime alone = first;
    StepCarSlide(&first, 0, 0);
    CHECK(first.slideInput == 193 && first.yawRate == -96);
    StepCarSlide(&alone, 0, 0);
    for (int i = 0; i < 40; i++) {
        StepCarSlide(&second, 0, 1);
        StepCarSlide(&first, 0, 0);
        StepCarSlide(&alone, 0, 0);
    }
    CHECK(memcmp(&first, &alone, sizeof(first)) == 0);
    CHECK(first.yawRate == -700 && second.yawRate == 700);
    GameCarRuntime restored = first;
    StepCarSlide(&first, 0, 0);
    StepCarSlide(&second, 0, 1);
    StepCarSlide(&restored, 0, 0);
    CHECK(memcmp(&first, &restored, sizeof(first)) == 0);
    first.slideInput = 0;
    first.slideActive = 1;
    for (int i = 0; i < 200; i++) StepCarSlide(&first, 0, 0);
    CHECK(first.yawRate == 0 && first.slideActive == 0);
    return 0;
}
