#include "common.h"
#include "game/car.h"
#include "game/track.h"

#include <stdio.h>
#include <string.h>

GameTrackPoint *g_TrackPoints;
s32 g_TrackPointCount;

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
    size_t i;
    int failures = 0;

    memset(points, 0, sizeof(points));
    points[1].leftHalfWidth = 120;
    points[1].rightHalfWidth = 80;
    g_TrackPoints = points;
    g_TrackPointCount = 2;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        GameCarRuntime car;

        memset(&car, 0, sizeof(car));
        car.trackPointIndex = 3; /* TrackPoint must wrap this to points[1]. */
        car.aiLateralOffset = cases[i].input;
        ClampCarLateralOffset(&car, cases[i].carIndex);
        if (car.aiLateralOffset != cases[i].expected) {
            printf("FAIL %s: got %d, expected %d\n", cases[i].label,
                   car.aiLateralOffset, cases[i].expected);
            failures++;
        }
    }

    if (failures != 0) return 1;
    puts("car lateral offsets stay within their side of the racing line");
    return 0;
}
