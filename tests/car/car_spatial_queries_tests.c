#include "common.h"
#include "game/angle.h"
#include "game/car.h"
#include "game/car_track_internal.h"
#include "game/track.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

static GameTrackPoint s_trackPoint;

static int TestFacingBackwards(void) {
    GameTrackPoint points[2];
    PlayerCarRuntime car;
    int angle;
    TrackRoute route = {0};

    memset(&car, 0, sizeof(car));
    memset(points, 0, sizeof(points));
    route.points = &s_trackPoint;
    route.count = 1;
    s_trackPoint.angle = 0x235;

    for (angle = 0; angle <= ANGLE_MASK; angle++) {
        s32 trackHeading = ANGLE_THREE_QUARTER_TURN - s_trackPoint.angle;
        s32 delta = (angle - trackHeading) & ANGLE_MASK;
        s32 expected = delta > ANGLE_QUARTER_TURN &&
                       delta < ANGLE_THREE_QUARTER_TURN;

        car.headingAngle = angle;
        if (CarFacesBackwards(&car, &route) != expected) {
            printf("FAIL facing at angle %#x, delta %#x\n", angle, delta);
            return 0;
        }
    }

    route.points = NULL;
    if (CarFacesBackwards(&car, &route) != 0) {
        puts("FAIL missing track data reports backwards");
        return 0;
    }
    route.points = points;
    route.count = 0;
    if (CarFacesBackwards(&car, &route) != 0) {
        puts("FAIL empty track reports backwards");
        return 0;
    }

    route.count = 2;
    points[0].angle = 0;
    points[1].angle = 0x400;
    car.trackPointIndex = -1;
    car.headingAngle = ANGLE_THREE_QUARTER_TURN - points[1].angle +
                       ANGLE_HALF_TURN;
    if (CarFacesBackwards(&car, &route) != 1) {
        puts("FAIL negative track index did not wrap");
        return 0;
    }

    route.points = &s_trackPoint;
    route.count = 1;
    s_trackPoint.angle = ANGLE_QUARTER_TURN;
    car.trackPointIndex = 0;
    car.headingAngle = INT32_MIN;
    if (CarFacesBackwards(&car, &route) != 1) {
        puts("FAIL minimum heading did not wrap into the angle domain");
        return 0;
    }
    car.headingAngle = INT32_MAX;
    if (CarFacesBackwards(&car, &route) != 1) {
        puts("FAIL maximum heading did not wrap into the angle domain");
        return 0;
    }
    points[0].angle = ANGLE_HALF_TURN;
    const TrackRoute other = {.points = points, .count = 2};
    car.headingAngle = ANGLE_THREE_QUARTER_TURN;
    if (CarFacesBackwards(&car, &other) != 1 ||
        CarFacesBackwards(&car, &route) != 0 ||
        CarFacesBackwards(&car, NULL) != 0 ||
        CarFacesBackwards(NULL, &route) != 0) {
        return 0;
    }
    return 1;
}

int main(void) {
    if (!TestFacingBackwards()) {
        return 1;
    }
    puts("car spatial queries passed");
    return 0;
}
