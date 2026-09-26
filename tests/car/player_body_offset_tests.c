#include "game/car.h"
#include "game/car_motion_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

static int s_failures;

#define CHECK(condition) do {                                                \
    if (!(condition)) {                                                      \
        printf("FAIL line %d: %s\n", __LINE__, #condition);                 \
        s_failures++;                                                        \
    }                                                                        \
} while (0)

int main(void) {
    PlayerCarRuntime car;

    memset(&car, 0, sizeof(car));
    car.reserved1C = 0x12345678;
    CalculatePlayerBodyOffset(&car);
    CHECK(car.motionX == 0 && car.motionY == 0 && car.motionZ == -50);
    CHECK(car.reserved1C == 0x12345678);

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

    memset(&car, 0, sizeof(car));
    car.drive.bodyLiftOffset = INT16_MAX;
    CalculatePlayerBodyOffset(&car);
    CHECK(car.motionX == 0 && car.motionY == 0 && car.motionZ == 32719);

    /* Baseline measured on the original matrix path before extraction. */
    const s32 angles[] = {0, 0x100, 0x400, -0x100, 0x700, 0xC00, INT_MAX, INT_MIN};
    const s32 lifts[] = {-50, 0, 25, INT16_MIN, INT16_MAX, INT_MAX};
    u32 digest = 2166136261U;
    for (int yaw = 0; yaw < 8; yaw++)
    for (int pitch = 0; pitch < 8; pitch++)
    for (int roll = 0; roll < 8; roll++)
    for (int lift = 0; lift < 6; lift++) {
        memset(&car, 0, sizeof(car));
        car.bodyYaw = angles[yaw];
        car.bodyPitch = angles[pitch];
        car.bodyRoll = angles[roll];
        car.drive.bodyLiftOffset = lifts[lift];
        CalculatePlayerBodyOffset(&car);
        const s32 values[] = {car.motionX, car.motionY, car.motionZ};
        for (int value = 0; value < 3; value++)
        for (int byte = 0; byte < 4; byte++) {
            digest ^= ((u32)values[value] >> (byte * 8)) & 255;
            digest *= 16777619U;
        }
    }
    if (digest != 121137701U) {
        printf("body offset sweep: %u\n", digest);
        s_failures++;
    }
    memset(&car, 0, sizeof(car));
    car.x = 100;
    car.z = 200;
    car.motionX = 10;
    car.motionZ = 20;
    car.drive.accelPos = 1280;
    car.drive.brakePos = -1280;
    IntegratePlayerPosition(&car);
    CHECK(car.x == 96 && car.z == 124);
    CHECK(car.motionX == 0 && car.motionZ == -50);
    PlayerCarRuntime isolated = car;
    PlayerCarRuntime other = {0};
    other.bodyYaw = 0x400;
    other.x = 1000;
    other.z = 2000;
    IntegratePlayerPosition(&other);
    CHECK(memcmp(&car, &isolated, sizeof(car)) == 0);
    memset(&car, 0, sizeof(car));
    car.x = INT_MIN;
    car.motionX = 1;
    car.drive.accelPos = INT_MAX;
    IntegratePlayerPosition(&car);
    CHECK(car.x == INT_MAX); /* The multiplied pedal contribution truncates to 0. */

    if (s_failures != 0) {
        printf("%d player body offset checks failed\n", s_failures);
        return 1;
    }
    puts("player body offset uses the inverse body rotation");
    return 0;
}
