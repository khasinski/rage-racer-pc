#include "game/car.h"
#include "game/angle.h"
#include "game/render.h"
#include "game/race_internal.h"
#include "game/track.h"

static s32 FinishCameraTargetPoint(const GameRenderObject *target) {
    s32 offset = target->facingBackwards != 0 ? 2 : -2;
    return WrapTrackPointIndex(g_CameraCarTrackPoint + offset);
}

/* Follow the centre line while keeping the finished car in view. */
void UpdateFinishCamera(PlayerCarRuntime *car) {
    GameRenderObject *obj = GetCarRenderObject(AsRivalCar(car));
    GameViewWork viewWork;
    s32 delta[3];
    s32 target[3];
    CarTrackLimits trackLimits = {0};
    s32 targetPoint;
    s32 targetHeading;
    s32 distance;

    if (g_TrackPoints == NULL || g_TrackPointCount <= 0) {
        return;
    }

    LoadViewWork(&viewWork);
    targetPoint = FinishCameraTargetPoint(obj);
    InterpolateTrackPoint(targetPoint, target, g_CameraCar.segmentFraction);
    targetHeading = ANGLE_QUARTER_TURN -
        Atan2(target[0] - g_CameraCar.x, target[2] - g_CameraCarZ);
    g_CameraCarHeading +=
        GetAngleDelta(g_CameraCarHeading, targetHeading);

    g_CameraCarStepX =
        (rsin(g_CameraCarHeading) * g_CameraCarSpeed) / 256;
    g_CameraCarStepZ =
        (rcos(g_CameraCarHeading) * g_CameraCarSpeed) / 256;
    g_CameraCar.x += g_CameraCarStepX / 256;
    g_CameraCarZ += g_CameraCarStepZ / 256;

    AccumulateLapProgress(&g_CameraCar);
    UpdateCarTrackState(&g_CameraCar, g_CameraCarTrackPoint, &trackLimits);

    viewWork.x = g_CameraCar.x;
    viewWork.y = g_CameraCar.y - 64;
    viewWork.z = g_CameraCar.z;
    viewWork.parameter = g_CameraCar.positionW;

    delta[0] = obj->x - viewWork.x;
    delta[1] = obj->y - viewWork.y;
    delta[2] = obj->z - viewWork.z;
    viewWork.angleY = ANGLE_QUARTER_TURN - Atan2(delta[0], delta[2]);
    distance = DistanceXZ(delta[0], delta[2]);
    viewWork.angleX = ANGLE_QUARTER_TURN - Atan2(delta[1], distance >> 6);
    viewWork.angleZ = 0;

    StoreViewWork(&viewWork);
    SetCameraRotMatrix();
    SelectModelBank(0);
    DrawPlayerCarModel(obj);
}
