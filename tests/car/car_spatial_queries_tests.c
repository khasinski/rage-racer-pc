#include "common.h"
#include "game/car.h"
#include "game/track.h"

#include <stdio.h>
#include <string.h>

static GameTrackPoint s_trackPoint;

static s32 PackPoint(s16 x, s16 y) {
    return (u16)x | ((s32)(u16)y << 16);
}

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

static int TestPointInQuad(void) {
    s32 topLeft = PackPoint(10, 10);
    s32 topRight = PackPoint(30, 10);
    s32 bottomLeft = PackPoint(10, 30);
    s32 bottomRight = PackPoint(30, 30);
    static const s16 inside[][2] = {
        {20, 20}, {10, 10}, {30, 30}, {20, 10}, {10, 20}
    };
    static const s16 outside[][2] = {
        {9, 20}, {31, 20}, {20, 9}, {20, 31}, {9, 9}, {31, 31}
    };
    size_t i;

    for (i = 0; i < sizeof(inside) / sizeof(inside[0]); i++) {
        if (!IsPointInQuad(topLeft, topRight, bottomLeft, bottomRight,
                           PackPoint(inside[i][0], inside[i][1]))) {
            printf("FAIL inside point %d,%d\n", inside[i][0], inside[i][1]);
            return 0;
        }
    }
    for (i = 0; i < sizeof(outside) / sizeof(outside[0]); i++) {
        if (IsPointInQuad(topLeft, topRight, bottomLeft, bottomRight,
                          PackPoint(outside[i][0], outside[i][1]))) {
            printf("FAIL outside point %d,%d\n", outside[i][0], outside[i][1]);
            return 0;
        }
    }
    return 1;
}

int main(void) {
    if (!TestFacingBackwards() || !TestPointInQuad()) {
        return 1;
    }
    puts("car spatial queries passed");
    return 0;
}
