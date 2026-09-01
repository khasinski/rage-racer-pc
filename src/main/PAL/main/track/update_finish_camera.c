#include "game/car.h"
#include "game/render.h"
#include "game/track.h"


/*
 * Camera track-follower: advances a look-ahead track point, aims the eye object
 * g_CameraCar toward the sampled centre-line point
 * (InterpolateTrackPoint + atan2), nudges its position, then seeds the camera
 * from the eye object, as a position and a pitch, yaw and roll, and submits
 * the render object (DrawPlayerCarModel). markerClamp is
 * the complete zeroed track-limit record passed to UpdateCarTrackState.
 */
void UpdateFinishCamera(GameRenderObject *obj) {
    GameViewWork viewWork;
    GameViewWork *view;
    s32 delta[3];
    s32 coords[3];
    CarTrackLimits markerClamp;
    s32 index;
    s32 rem;
    s32 offset;
    s32 angle;
    s32 value;
    s32 zValue;
    GameCarRuntimeAddress cameraAddress;
    GameBlockAddress viewAddress;

    LoadViewWork(&viewWork);
    view = &viewWork;
    offset = g_CameraCarTrackPoint;
    if (obj->facingBackwards != 0) {
        index = offset + 2;
    } else {
        index = offset - 2;
    }
    rem = index;
    if (index < 0) {
        rem = index + g_TrackPointCount;
    }
    index = rem % g_TrackPointCount;

    InterpolateTrackPoint(index, coords, g_CameraCar.segmentFraction);
    angle = 0x400 - Atan2(coords[0] - g_CameraCar.x, coords[2] - g_CameraCarZ);

    g_CameraCarHeading += GetAngleDelta(g_CameraCarHeading, angle);

    value = rsin(g_CameraCarHeading) * g_CameraCarSpeed;
    g_CameraCarStepX = value / 256;

    zValue = rcos(g_CameraCarHeading) * g_CameraCarSpeed;
    g_CameraCarStepZ = zValue / 256;

    g_CameraCar.x = g_CameraCarStepX / 256 + g_CameraCar.x;
    g_CameraCarZ = g_CameraCarStepZ / 256 + g_CameraCarZ;

    AccumulateLapProgress(&g_CameraCar);
    markerClamp.rightInset = 0;
    markerClamp.leftInset = 0;
    markerClamp.rightKnockbackMode = 0;
    markerClamp.leftKnockbackMode = 0;
    UpdateCarTrackState(&g_CameraCar, g_CameraCarTrackPoint, &markerClamp);

    cameraAddress.runtime = &g_CameraCar;
    viewAddress.words = &view->x;
    viewAddress.blocks[0] = cameraAddress.blocks[0];
    view->y -= 64;

    delta[0] = obj->x - view->x;
    delta[1] = obj->y - view->y;
    delta[2] = obj->z - view->z;

    view->angleY = 0x400 - Atan2(delta[0], delta[2]);
    value = DistanceXZ(delta[0], delta[2]);
    view->angleX = 0x400 - Atan2(delta[1], value >> 6);
    view->angleZ = 0;

    StoreViewWork(&viewWork);
    SetCameraRotMatrix();
    SelectModelBank(0);
    DrawPlayerCarModel(obj);
}
