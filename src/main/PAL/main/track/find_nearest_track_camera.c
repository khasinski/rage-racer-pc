#include "game/render.h"

#include "game/track_camera_internal.h"

s32 FindNearestTrackCamera(GameRenderObject *car) {
    s32 bestDistance = 0x7FFFFFFF;
    s32 selected = 0;
    s32 trackLength = (s16)g_TrackSectionCount;
    s32 target = car->trackSection;
    s32 index;

    for (index = 0; g_TrackCameras[index].trackSection.value != -1; index++) {
        s32 cameraSection = g_TrackCameras[index].trackSection.value;
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
