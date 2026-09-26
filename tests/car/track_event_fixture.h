#ifndef TRACK_EVENT_FIXTURE_H
#define TRACK_EVENT_FIXTURE_H
#include "game/car.h"
#include "game/track.h"
static void SeedValidTrackEventTables(TrackEventData *data) {
    u8 *base = (u8 *)&data->offsets;
    SceneryMotionData *flyby =
        (SceneryMotionData *)(base + data->offsets.flybyScenery);
    SceneryMotionData *route =
        (SceneryMotionData *)(base + data->offsets.routeScenery);
    RaceIntroCameraScript *camera =
        (RaceIntroCameraScript *)(base + data->offsets.raceIntroCamera);
    PathSceneryPositionData *position =
        (PathSceneryPositionData *)(base + data->offsets.pathSceneryPosition);
    PathSceneryRotationData *rotation =
        (PathSceneryRotationData *)(base + data->offsets.pathSceneryRotation);

    flyby->keyframes[0].duration = 1;
    flyby->keyframes[1].duration = SCENERY_MOTION_END;
    route->keyframes[0].duration = 1;
    route->keyframes[1].duration = SCENERY_MOTION_END;
    camera->keys[0].mode = 1;
    camera->keys[0].duration = 0;
    position->keys[0].span = 0;
    position->keys[1].span = -1;
    position->keys[1].loopIndex = 0;
    rotation->keys[0].fields.span = 0;
    rotation->keys[1].fields.span = -1;
    rotation->keys[1].fields.loopIndex = 0;
}

#endif
