#include "game/race.h"
#include "game/track_camera_internal.h"

#include <stddef.h>
#include <stdio.h>

s16 g_GrandPrixSeries;
const GameTrackCameraNode *g_TrackCameras;

typedef struct CameraTableFixture {
    TrackCameraTable table;
    GameTrackCameraNode defaultCamera;
    GameTrackCameraNode firstSeriesCamera;
    GameTrackCameraNode secondSeriesCamera;
} CameraTableFixture;

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__,   \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    CameraTableFixture fixture = {0};

    fixture.table.defaultOffset = offsetof(CameraTableFixture, defaultCamera);
    fixture.table.seriesOffset[0] =
        offsetof(CameraTableFixture, firstSeriesCamera);
    fixture.table.seriesOffset[1] =
        offsetof(CameraTableFixture, secondSeriesCamera);
    fixture.defaultCamera.trackSection.value = -1;
    fixture.firstSeriesCamera.trackSection.value = -1;
    fixture.secondSeriesCamera.trackSection.value = -1;

    g_GrandPrixSeries = 1;
    CHECK(SelectTrackCameraTable(&fixture.table, sizeof(fixture), 0) == 1);
    CHECK(g_TrackCameras == &fixture.defaultCamera);

    g_GrandPrixSeries = 0;
    CHECK(SelectTrackCameraTable(&fixture.table, sizeof(fixture), 1) == 1);
    CHECK(g_TrackCameras == &fixture.firstSeriesCamera);

    g_GrandPrixSeries = 4;
    CHECK(IsValidTrackCameraTable(&fixture.table, sizeof(fixture), 1) == 1);
    CHECK(SelectTrackCameraTable(&fixture.table, sizeof(fixture), 1) == 1);
    CHECK(g_TrackCameras == &fixture.secondSeriesCamera);

    CHECK(IsValidTrackCameraTable(NULL, 0, 0) == 0);
    CHECK(SelectTrackCameraTable(NULL, 0, 0) == 0);
    CHECK(g_TrackCameras == NULL);

    fixture.table.defaultOffset = -1;
    CHECK(SelectTrackCameraTable(&fixture.table, sizeof(fixture), 0) == 0);
    CHECK(g_TrackCameras == NULL);

    fixture.table.defaultOffset = sizeof(fixture.table) - sizeof(s32);
    CHECK(SelectTrackCameraTable(&fixture.table, sizeof(fixture), 0) == 0);
    CHECK(g_TrackCameras == NULL);

    fixture.table.defaultOffset = sizeof(fixture.table) + 1;
    CHECK(SelectTrackCameraTable(&fixture.table, sizeof(fixture), 0) == 0);
    CHECK(g_TrackCameras == NULL);

    fixture.table.defaultOffset = offsetof(CameraTableFixture, defaultCamera);
    fixture.defaultCamera.trackSection.value = 0;
    fixture.defaultCamera.mode = TRACK_CAMERA_ORBIT + 1;
    CHECK(SelectTrackCameraTable(&fixture.table, sizeof(fixture), 0) == 0);
    CHECK(g_TrackCameras == NULL);

    fixture.defaultCamera.mode = TRACK_CAMERA_CAR;
    CHECK(SelectTrackCameraTable(
              &fixture.table,
              offsetof(CameraTableFixture, firstSeriesCamera), 0) == 0);
    CHECK(g_TrackCameras == NULL);

    puts("track camera table selection tests passed");
    return 0;
}
