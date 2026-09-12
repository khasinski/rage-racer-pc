#include "game/car.h"
#include "game/angle.h"
#include "game/render.h"
#include "game/race_internal.h"
#include "game/track.h"

static s32 FinishCameraTargetPoint(const GameCarRuntime *target) {
    s32 offset = target->facingBackwards != 0 ? 2 : -2;
    return WrapTrackPointIndex(WrapSigned32(
        (int64_t)g_CameraCarTrackPoint + offset));
}

/* Follow the centre line while keeping the finished car in view. */
void UpdateFinishCamera(Camera *camera, PlayerCarRuntime *car) {
    GameCarRuntime *obj = AsRivalCar(car);
    GameViewWork viewWork;
    LVec delta;
    LVec target;
    CarTrackLimits trackLimits = {0};
    s32 targetPoint;
    s32 targetHeading;
    s32 distance;

    if (g_TrackPoints == NULL || g_TrackPointCount <= 0) {
        return;
    }

    LoadViewWork(&viewWork, &camera->view);
    targetPoint = FinishCameraTargetPoint(obj);
    InterpolateTrackPoint(targetPoint, &target, g_CameraCar.segmentFraction);
    targetHeading = ANGLE_QUARTER_TURN - Atan2(
        WrapSigned32((int64_t)target.x - g_CameraCar.x),
        WrapSigned32((int64_t)target.z - g_CameraCarZ));
    g_CameraCarHeading = WrapSigned32(
        (int64_t)g_CameraCarHeading +
        GetAngleDelta(g_CameraCarHeading, targetHeading));

    g_CameraCarStepX = WrapSigned32(
        (int64_t)rsin(g_CameraCarHeading) * g_CameraCarSpeed) / 256;
    g_CameraCarStepZ = WrapSigned32(
        (int64_t)rcos(g_CameraCarHeading) * g_CameraCarSpeed) / 256;
    g_CameraCar.x = WrapSigned32(
        (int64_t)g_CameraCar.x + g_CameraCarStepX / 256);
    g_CameraCarZ = WrapSigned32(
        (int64_t)g_CameraCarZ + g_CameraCarStepZ / 256);

    AccumulateLapProgress(&g_CameraCar);
    UpdateCarTrackState(&g_CameraCar, g_CameraCarTrackPoint, &trackLimits);

    viewWork.x = g_CameraCar.x;
    viewWork.y = WrapSigned32((int64_t)g_CameraCar.y - 64);
    viewWork.z = g_CameraCar.z;
    viewWork.parameter = g_CameraCar.positionW;

    delta.x = WrapSigned32((int64_t)obj->x - viewWork.x);
    delta.y = WrapSigned32((int64_t)obj->y - viewWork.y);
    delta.z = WrapSigned32((int64_t)obj->z - viewWork.z);
    viewWork.angleY = ANGLE_QUARTER_TURN - Atan2(delta.x, delta.z);
    distance = DistanceXZ(delta.x, delta.z);
    viewWork.angleX = ANGLE_QUARTER_TURN - Atan2(delta.y, distance >> 6);
    viewWork.angleZ = 0;

    StoreViewWork(&camera->view, &viewWork);
    SetCameraRotMatrix(&camera->view);
    SelectModelBank(0);
    DrawPlayerCarModel(obj);
}
