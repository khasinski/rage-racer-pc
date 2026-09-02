#include "common.h"
#include "game/car.h"
#include "game/track.h"

#include <stdio.h>
#include <string.h>

static GameTrackPoint s_trackPoint;

static int TestFacingBackwards(void) {
    PlayerCarRuntime car;
    int angle;

    memset(&car, 0, sizeof(car));
    g_TrackPoints = &s_trackPoint;
    g_TrackPointCount = 1;
    s_trackPoint.angle = 0x235;

    for (angle = 0; angle < 0x1000; angle++) {
        s32 trackHeading = 0xC00 - s_trackPoint.angle;
        s32 delta = (angle - trackHeading) & 0xFFF;
        s32 expected = delta > 0x400 && delta < 0xC00;

        car.headingAngle = angle;
        if (IsCarFacingBackwards(&car) != expected) {
            printf("FAIL facing at angle %#x, delta %#x\n", angle, delta);
            return 0;
        }
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
