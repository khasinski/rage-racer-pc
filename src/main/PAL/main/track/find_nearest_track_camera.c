#include "game/render.h"

#include "game/track_camera_internal.h"

static s32 WrapTrackSection(s32 section, s32 count) {
    section %= count;
    return section < 0 ? section + count : section;
}

s32 FindNearestTrackCamera(GameRenderObject *car) {
    s32 bestDistance = 0x7FFFFFFF;
    s32 selected = 0;
    s32 trackLength = g_TrackSectionCount;
    s32 target;
    s32 index;

    if (g_TrackCameras == NULL || trackLength <= 0 ||
        g_TrackCameras[0].trackSection.value == -1) {
        return -1;
    }
    target = WrapTrackSection(car->trackSection, trackLength);

    for (index = 0; g_TrackCameras[index].trackSection.value != -1; index++) {
        s32 cameraSection = WrapTrackSection(
            g_TrackCameras[index].trackSection.value, trackLength);
        s32 distance = cameraSection - target;

        if (distance < 0) distance = -distance;
        if (distance > trackLength / 2) distance = trackLength - distance;
        if (distance < bestDistance) {
            selected = index;
            bestDistance = distance;
        }
    }
    return selected;
}
