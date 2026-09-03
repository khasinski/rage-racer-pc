#include "game/render.h"
#include "game/track.h"
#include "game/track_camera_internal.h"

#include <stdio.h>

const GameTrackCameraNode *g_TrackCameras;
u16 g_TrackSectionCount;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", \
                __FILE__, __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void) {
    GameTrackCameraNode nodes[5] = {0};
    GameRenderObject car = {0};

    g_TrackCameras = nodes;
    g_TrackSectionCount = 100;
    nodes[0].trackSection.value = 10;
    nodes[1].trackSection.value = 40;
    nodes[2].trackSection.value = 95;
    nodes[3].trackSection.value = -1;

    car.trackSection = 43;
    CHECK(FindNearestTrackCamera(&car) == 1);

    /* Section 95 is only seven sections away through the ring boundary. */
    car.trackSection = 2;
    CHECK(FindNearestTrackCamera(&car) == 2);

    /* Equal distances retain the earlier authored camera. */
    nodes[0].trackSection.value = 20;
    nodes[1].trackSection.value = 40;
    nodes[2].trackSection.value = -1;
    car.trackSection = 30;
    CHECK(FindNearestTrackCamera(&car) == 0);

    nodes[0].trackSection.value = -1;
    CHECK(FindNearestTrackCamera(&car) == -1);

    g_TrackCameras = NULL;
    CHECK(FindNearestTrackCamera(&car) == -1);

    g_TrackCameras = nodes;
    nodes[0].trackSection.value = 10;
    g_TrackSectionCount = 0;
    CHECK(FindNearestTrackCamera(&car) == -1);

    g_TrackSectionCount = 100;
    nodes[0].trackSection.value = 140;
    nodes[1].trackSection.value = -5;
    nodes[2].trackSection.value = -1;
    car.trackSection = 102;
    CHECK(FindNearestTrackCamera(&car) == 1);

    /* Exercise the largest section ring representable by runtime s16
     * positions without narrowing its unsigned count during arithmetic. */
    g_TrackSectionCount = INT16_MAX;
    nodes[0].trackSection.value = 30000;
    nodes[1].trackSection.value = -30000;
    nodes[2].trackSection.value = -1;
    car.trackSection = INT16_MAX - 1;
    CHECK(FindNearestTrackCamera(&car) == 0);

    puts("find_nearest_track_camera: linear, wrapped and tied distances ok");
    return 0;
}
