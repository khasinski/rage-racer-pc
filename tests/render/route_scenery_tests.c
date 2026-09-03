#include "common.h"
#include "game/race.h"
#include "game/render.h"
#include "game/track_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

s32 g_RaceSeries;
const SceneryMotionData *g_RouteSceneryData;
s32 g_RouteSceneryActive;
s32 g_RouteSceneryFrame;
s16 g_RouteSceneryKeyIndex;
s32 g_RouteSceneryRotX;
s32 g_RouteSceneryRotY;
s32 g_RouteSceneryRotZ;
const SceneryMotionKeyframe *g_RouteSceneryKeyframe;
Vec4 g_RouteSceneryPosition;

static void BuildIdentity(void *matrix) {
    Matrix *mtx = matrix;

    memset(mtx, 0, sizeof(*mtx));
    mtx->m[0][0] = 0x1000;
    mtx->m[1][1] = 0x1000;
    mtx->m[2][2] = 0x1000;
}

void BuildRotMatrixX(void *matrix, s32 angle) {
    (void)angle;
    BuildIdentity(matrix);
}

void BuildRotMatrixY(void *matrix, s32 angle) {
    (void)angle;
    BuildIdentity(matrix);
}

void BuildRotMatrixZ(void *matrix, s32 angle) {
    (void)angle;
    BuildIdentity(matrix);
}

typedef struct RouteSceneryFixture {
    s16 triggerSection[2][2];
    s16 firstKeyframe[2][2];
    SceneryMotionStart start[2];
    SceneryMotionKeyframe keyframes[3];
} RouteSceneryFixture;

static int CheckState(const char *label, s32 clock, s32 frame, s16 key,
                      s32 rotX, s32 rotY, s32 rotZ) {
    if (g_RouteSceneryActive != clock || g_RouteSceneryFrame != frame ||
        g_RouteSceneryKeyIndex != key || g_RouteSceneryRotX != rotX ||
        g_RouteSceneryRotY != rotY || g_RouteSceneryRotZ != rotZ) {
        printf("FAIL %s: clock=%d frame=%d key=%d rot=(%d,%d,%d)\n",
               label, g_RouteSceneryActive, g_RouteSceneryFrame,
               g_RouteSceneryKeyIndex, g_RouteSceneryRotX,
               g_RouteSceneryRotY, g_RouteSceneryRotZ);
        return 0;
    }
    return 1;
}

int main(void) {
    RouteSceneryFixture fixture;

    memset(&fixture, 0, sizeof(fixture));
    fixture.firstKeyframe[1][0] = 0;
    fixture.start[1].position = (Vec4){1000, 2000, 3000, 4000};
    fixture.keyframes[0] =
        (SceneryMotionKeyframe){100, 200, 300, 2, 0, 0};
    fixture.keyframes[1] =
        (SceneryMotionKeyframe){300, 600, 900, 2, 0, 0};
    fixture.keyframes[2] =
        (SceneryMotionKeyframe){500, 1000, 1500, -1, 0, 0};

    g_RaceSeries = 7;
    g_RouteSceneryData = (SceneryMotionData *)&fixture;
    g_RouteSceneryPosition = (Vec4){10, 20, 30, 40};

    UpdateRouteScenery();
    if (!CheckState("inactive", 0, 0, 0, 0, 0, 0) ||
        g_RouteSceneryPosition.x != 10) {
        return 1;
    }

    SeedRouteScenery();
    if (!CheckState("seed", 1, 0, 0, 100, 200, 300) ||
        g_RouteSceneryKeyframe != fixture.keyframes) {
        puts("FAIL seed: route scenery was not initialized");
        return 1;
    }
    UpdateRouteScenery();
    if (!CheckState("first half", 1, 1, 0, 200, 400, 600)) {
        return 1;
    }
    UpdateRouteScenery();
    if (!CheckState("second key", 1, 0, 1, 300, 600, 900)) {
        return 1;
    }
    UpdateRouteScenery();
    if (!CheckState("second half", 1, 1, 1, 400, 800, 1200)) {
        return 1;
    }
    UpdateRouteScenery();
    if (!CheckState("loop", 1, 0, 0, 100, 200, 300) ||
        memcmp(&g_RouteSceneryPosition, &fixture.start[1].position,
               sizeof(g_RouteSceneryPosition)) != 0) {
        puts("FAIL loop: start position was not restored");
        return 1;
    }

    fixture.keyframes[0].speed = 1;
    g_RouteSceneryPosition.z = INT_MIN;
    UpdateRouteScenery();
    if (g_RouteSceneryPosition.z != INT_MAX) {
        puts("FAIL wrapped route scenery position");
        return 1;
    }

    puts("route scenery keyframe progression preserved");
    return 0;
}
