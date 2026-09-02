#include "game/car.h"
#include "game/car_internal.h"
#include "game/render_state.h"

#include <stdio.h>
#include <string.h>

static int s_failures;

GameRenderState g_RenderState;

MATRIX *MulMatrix0(MATRIX *left, MATRIX *right, MATRIX *output) {
    (void)left;
    (void)right;
    return output;
}

void GameRenderWorldSetCamera(int32_t x, int32_t y, int32_t z,
                              int32_t pitch, int32_t yaw, int32_t roll) {
    (void)x;
    (void)y;
    (void)z;
    (void)pitch;
    (void)yaw;
    (void)roll;
}

#define CHECK(condition) do {                                                \
    if (!(condition)) {                                                      \
        printf("FAIL line %d: %s\n", __LINE__, #condition);                 \
        s_failures++;                                                        \
    }                                                                        \
} while (0)

int main(void) {
    PlayerCarRuntime car;

    memset(&car, 0, sizeof(car));
    CalculatePlayerBodyOffset(&car);
    CHECK(car.motionX == 0 && car.motionY == 0 && car.motionZ == -50);

    memset(&car, 0, sizeof(car));
    car.drive.bodyLiftOffset = 25;
    CalculatePlayerBodyOffset(&car);
    CHECK(car.motionX == 0 && car.motionY == 0 && car.motionZ == -75);

    memset(&car, 0, sizeof(car));
    car.bodyYaw = 0x400;
    car.drive.bodyLiftOffset = 25;
    CalculatePlayerBodyOffset(&car);
    CHECK(car.motionY == 0);
    CHECK(car.motionX * car.motionX + car.motionZ * car.motionZ >= 74 * 74);
    CHECK(car.motionX * car.motionX + car.motionZ * car.motionZ <= 76 * 76);

    memset(&car, 0, sizeof(car));
    car.bodyPitch = 0x400;
    car.drive.bodyLiftOffset = 25;
    CalculatePlayerBodyOffset(&car);
    CHECK(car.motionX == 0);
    CHECK(car.motionY * car.motionY + car.motionZ * car.motionZ >= 74 * 74);
    CHECK(car.motionY * car.motionY + car.motionZ * car.motionZ <= 76 * 76);

    memset(&car, 0, sizeof(car));
    car.bodyPitch = 0x200;
    car.bodyYaw = 0x200;
    car.bodyRoll = 0x200;
    car.drive.bodyLiftOffset = 25;
    CalculatePlayerBodyOffset(&car);
    CHECK(car.motionX != 0 && car.motionY != 0 && car.motionZ != 0);
    CHECK(car.motionX * car.motionX + car.motionY * car.motionY +
              car.motionZ * car.motionZ >=
          73 * 73);
    CHECK(car.motionX * car.motionX + car.motionY * car.motionY +
              car.motionZ * car.motionZ <=
          77 * 77);

    if (s_failures != 0) {
        printf("%d player body offset checks failed\n", s_failures);
        return 1;
    }
    puts("player body offset uses the inverse body rotation");
    return 0;
}
