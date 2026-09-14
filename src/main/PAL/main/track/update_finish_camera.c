#include "game/car.h"
#include "game/angle.h"
#include "game/render.h"
#include "game/race_internal.h"
#include "game/track.h"

static s32 FinishCameraTargetPoint(const FinishCamera *finish,
                                   const GameCarRuntime *target) {
    s32 offset = target->facingBackwards != 0 ? 2 : -2;
    return WrapTrackPointIndex(WrapSigned32(
        (int64_t)finish->point + offset));
}

/* Follow the centre line while keeping the finished car in view. */
void UpdateFinishCamera(Camera *camera, FinishCamera *finish,
                        PlayerCarRuntime *car) {
    GameCarRuntime *obj = AsRivalCar(car);
    GameViewWork viewWork;
    LVec delta;
    LVec target;
    CarTrackLimits trackLimits = {0};
    s32 targetPoint;
    s32 targetHeading;
    s32 distance;
    s32 stepX;
    s32 stepZ;

    if (g_TrackPoints == NULL || g_TrackPointCount <= 0) {
        return;
    }

    LoadViewWork(&viewWork, &camera->view);
    targetPoint = FinishCameraTargetPoint(finish, obj);
    InterpolateTrackPoint(targetPoint, &target, finish->car.segmentFraction);
    targetHeading = ANGLE_QUARTER_TURN - Atan2(
        WrapSigned32((int64_t)target.x - finish->car.x),
        WrapSigned32((int64_t)target.z - finish->car.z));
    finish->heading = WrapSigned32(
        (int64_t)finish->heading +
        GetAngleDelta(finish->heading, targetHeading));

    stepX = WrapSigned32(
        (int64_t)rsin(finish->heading) * finish->car.speed) / 256;
    stepZ = WrapSigned32(
        (int64_t)rcos(finish->heading) * finish->car.speed) / 256;
    finish->car.x = WrapSigned32(
        (int64_t)finish->car.x + stepX / 256);
    finish->car.z = WrapSigned32(
        (int64_t)finish->car.z + stepZ / 256);

    AccumulateLapProgress(&finish->car);
    UpdateCarTrackState(&finish->car, finish->point, &trackLimits);

    viewWork.x = finish->car.x;
    viewWork.y = WrapSigned32((int64_t)finish->car.y - 64);
    viewWork.z = finish->car.z;
    viewWork.parameter = finish->car.positionW;

    delta.x = WrapSigned32((int64_t)obj->x - viewWork.x);
    delta.y = WrapSigned32((int64_t)obj->y - viewWork.y);
    delta.z = WrapSigned32((int64_t)obj->z - viewWork.z);
    viewWork.angleY = ANGLE_QUARTER_TURN - Atan2(delta.x, delta.z);
    distance = DistanceXZ(delta.x, delta.z);
    viewWork.angleX = ANGLE_QUARTER_TURN - Atan2(delta.y, distance >> 6);
    viewWork.angleZ = 0;

    StoreViewWork(&camera->view, &viewWork);
    SetCameraRotMatrix(&g_RenderState, &camera->view);
    SelectModelBank(0);
    DrawRacePlayerCarModel(obj);
}
