#include "camera_internal.h"

void CameraLoadViewPositionFromCar(GameViewWork *view,
                                    const GameRenderObject *car) {
    view->x = car->x;
    view->y = car->y;
    view->z = car->z;
    view->reserved = car->field_0C;
}

void CameraLoadViewPoseFromCar(GameViewWork *view,
                                const GameRenderObject *car) {
    CameraLoadViewPositionFromCar(view, car);
    view->angleX = car->bodyPitch;
    view->angleY = car->angleY;
    view->angleZ = car->bodyRoll;
    view->depth = car->field_2C;
}

/*
 * Settle the chase yaw for one frame. Four paths reach this: the yaw error
 * can be positive or negative, and either can be the short way round or the
 * long way across the wrap. They differ only in which ramp they charge and
 * which way the lag points, so the decompiler had two of them jump into the
 * bodies of the other two. `limit` is how far the camera may swing this
 * frame, `accel` how far the ramp wants to, and the smaller wins.
 */
