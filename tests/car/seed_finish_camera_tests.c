#include "common.h"
#include "game/car.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/track.h"

#include <stdio.h>
#include <string.h>

GameCarRuntime g_CameraCar;
GameTrackPoint *g_TrackPoints;
s32 g_TrackPointCount;
s32 g_CameraCarSeedYaw;

#define CHECK_EQ(actual, expected) do {                                        \
    if ((actual) != (expected)) {                                               \
        fprintf(stderr, "line %d: %s = %d, expected %d\n", __LINE__, #actual, \
                (int)(actual), (int)(expected));                                \
        return 1;                                                               \
    }                                                                           \
} while (0)

int main(void) {
    static GameTrackPoint points[3];
    PlayerCarRuntime car;

    memset(&car, 0, sizeof(car));
    memset(points, 0, sizeof(points));
    g_TrackPoints = points;
    g_TrackPointCount = 3;
    points[1].x = 100;
    points[1].y = 200;
    points[1].z = 300;
    points[1].angle = 0x120;
    car.trackPointIndex = 4;
    car.speed = 500;
    car.facingBackwards = 0;

    SeedFinishCamera(&car);
    CHECK_EQ(g_CameraCar.x, 100);
    CHECK_EQ(g_CameraCar.y, 136);
    CHECK_EQ(g_CameraCar.z, 300);
    CHECK_EQ(g_CameraCar.speed, 564);
    CHECK_EQ(g_CameraCar.headingAngle, 0xAE0);
    CHECK_EQ(g_CameraCar.bodyYaw, 0xAE0);
    CHECK_EQ(g_CameraCarSeedYaw, 0xAE0);

    car.facingBackwards = 1;
    SeedFinishCamera(&car);
    CHECK_EQ(g_CameraCar.headingAngle, 0x12E0);

    g_CameraCar.x = 777;
    g_TrackPointCount = 0;
    SeedFinishCamera(&car);
    CHECK_EQ(g_CameraCar.x, 777);

    puts("finish camera seeds from a wrapped track point");
    return 0;
}
