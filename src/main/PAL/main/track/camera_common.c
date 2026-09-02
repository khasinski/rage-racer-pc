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
