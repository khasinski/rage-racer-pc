#include "common.h"
#include "game/car.h"
#include "game/rival.h"
#include "game/track.h"

#include <stdio.h>
#include <string.h>


int main(void) {
    static const struct {
        const char *label;
        s32 carIndex;
        s16 input;
        s16 expected;
    } cases[] = {
        {"front group inside right", 3, 49, 49},
        {"front group on right limit", 3, 50, 50},
        {"front group beyond right", 3, 51, 50},
        {"front group inside left", 0, -74, -74},
        {"front group on left limit", 0, -75, -75},
        {"front group beyond left", 0, -76, -75},
        {"rear group inside right", 4, 44, 44},
        {"rear group on right limit", 8, 45, 45},
        {"rear group beyond right", 10, 46, 45},
        {"rear group inside left", 4, -67, -67},
        {"rear group on left limit", 8, -68, -68},
        {"rear group beyond left", 10, -69, -68},
        {"centre remains centred", 0, 0, 0},
    };
    GameTrackPoint points[2];
    GameCarRuntime car;
    TrackRoute route = {.points = points, .count = 2};
    size_t i;
    int failures = 0;

    memset(points, 0, sizeof(points));
    points[1].leftHalfWidth = 120;
    points[1].rightHalfWidth = 80;
    route.count = 2;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        memset(&car, 0, sizeof(car));
        car.trackPointIndex = 3; /* TrackPoint must wrap this to points[1]. */
        car.aiLateralOffset = cases[i].input;
        ClampRivalLine(&car, cases[i].carIndex, &route);
        if (car.aiLateralOffset != cases[i].expected) {
            printf("FAIL %s: got %d, expected %d\n", cases[i].label,
                   car.aiLateralOffset, cases[i].expected);
            failures++;
        }
    }

    memset(&car, 0, sizeof(car));
    car.aiLateralOffset = -123;
    route.count = 0;
    ClampRivalLine(&car, 0, &route);
    if (car.aiLateralOffset != -123) {
        puts("FAIL empty track changed the lateral offset");
        failures++;
    }

    route.count = 2;
    route.points = NULL;
    ClampRivalLine(&car, 0, &route);
    if (car.aiLateralOffset != -123) {
        puts("FAIL missing track data changed the lateral offset");
        failures++;
    }

    route.points = points;
    points[0].rightHalfWidth = -80;
    car.trackPointIndex = 0;
    car.aiLateralOffset = 25;
    ClampRivalLine(&car, 0, &route);
    if (car.aiLateralOffset != 0) {
        puts("FAIL negative track width reversed the lateral offset");
        failures++;
    }

    car.aiLateralOffset = 25;
    ClampRivalLine(&car, -1, &route);
    if (car.aiLateralOffset != 25) {
        puts("FAIL invalid rival slot changed the lateral offset");
        failures++;
    }

    if (failures != 0) {
        return 1;
    }
    puts("car lateral offsets stay within their side of the racing line");
    return 0;
}
