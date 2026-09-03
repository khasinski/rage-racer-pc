#include "common.h"
#include "game/angle.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/track.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

static GameTrackPoint s_trackPoint;

static int TestFacingBackwards(void) {
    GameTrackPoint points[2];
    PlayerCarRuntime car;
    int angle;

    memset(&car, 0, sizeof(car));
    memset(points, 0, sizeof(points));
    g_TrackPoints = &s_trackPoint;
    g_TrackPointCount = 1;
    s_trackPoint.angle = 0x235;

    for (angle = 0; angle <= ANGLE_MASK; angle++) {
        s32 trackHeading = ANGLE_THREE_QUARTER_TURN - s_trackPoint.angle;
        s32 delta = (angle - trackHeading) & ANGLE_MASK;
        s32 expected = delta > ANGLE_QUARTER_TURN &&
                       delta < ANGLE_THREE_QUARTER_TURN;

        car.headingAngle = angle;
        if (IsCarFacingBackwards(&car) != expected) {
            printf("FAIL facing at angle %#x, delta %#x\n", angle, delta);
            return 0;
        }
    }

    g_TrackPoints = NULL;
    if (IsCarFacingBackwards(&car) != 0) {
        puts("FAIL missing track data reports backwards");
        return 0;
    }
    g_TrackPoints = points;
    g_TrackPointCount = 0;
    if (IsCarFacingBackwards(&car) != 0) {
        puts("FAIL empty track reports backwards");
        return 0;
    }

    g_TrackPointCount = 2;
    points[0].angle = 0;
    points[1].angle = 0x400;
    car.trackPointIndex = -1;
    car.headingAngle = ANGLE_THREE_QUARTER_TURN - points[1].angle +
                       ANGLE_HALF_TURN;
    if (IsCarFacingBackwards(&car) != 1) {
        puts("FAIL negative track index did not wrap");
        return 0;
    }

    g_TrackPoints = &s_trackPoint;
    g_TrackPointCount = 1;
    s_trackPoint.angle = ANGLE_QUARTER_TURN;
    car.trackPointIndex = 0;
    car.headingAngle = INT32_MIN;
    if (IsCarFacingBackwards(&car) != 1) {
        puts("FAIL minimum heading did not wrap into the angle domain");
        return 0;
    }
    car.headingAngle = INT32_MAX;
    if (IsCarFacingBackwards(&car) != 1) {
        puts("FAIL maximum heading did not wrap into the angle domain");
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
