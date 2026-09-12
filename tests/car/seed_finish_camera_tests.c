#include "common.h"
#include "game/car.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/track.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

const GameTrackPoint *g_TrackPoints;
s32 g_TrackPointCount;

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
    FinishCamera finish;

    memset(&car, 0, sizeof(car));
    memset(&finish, 0, sizeof(finish));
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

    SeedFinishCamera(&finish, &car);
    CHECK_EQ(finish.car.x, 100);
    CHECK_EQ(finish.car.y, 136);
    CHECK_EQ(finish.car.z, 300);
    CHECK_EQ(finish.car.speed, 564);
    CHECK_EQ(finish.car.headingAngle, 0xAE0);
    CHECK_EQ(finish.car.bodyYaw, 0xAE0);
    CHECK_EQ(finish.seedYaw, 0xAE0);
    CHECK_EQ(finish.point, 4);
    CHECK_EQ(finish.heading, 0xAE0);
    CHECK_EQ(finish.section, 0);

    car.facingBackwards = 1;
    SeedFinishCamera(&finish, &car);
    CHECK_EQ(finish.car.headingAngle, 0x12E0);

    car.speed = INT_MAX;
    SeedFinishCamera(&finish, &car);
    CHECK_EQ(finish.car.speed, INT_MIN + 63);

    finish.car.x = 777;
    g_TrackPointCount = 0;
    SeedFinishCamera(&finish, &car);
    CHECK_EQ(finish.car.x, 777);

    g_TrackPointCount = 3;
    g_TrackPoints = NULL;
    SeedFinishCamera(&finish, &car);
    CHECK_EQ(finish.car.x, 777);

    g_TrackPoints = points;
    SeedFinishCamera(&finish, NULL);
    CHECK_EQ(finish.car.x, 777);

    car.facingBackwards = SHRT_MAX;
    SeedFinishCamera(&finish, &car);
    CHECK_EQ(finish.car.headingAngle, 0x12E0);

    puts("finish camera seeds from a wrapped track point");
    return 0;
}
