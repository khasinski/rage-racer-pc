#include "camera_internal.h"
#include "game/car.h"

void CameraLoadViewPositionFromCar(GameViewWork *view,
                                    const GameCarRuntime *car) {
    view->x = car->x;
    view->y = car->y;
    view->z = car->z;
    view->parameter = car->positionW;
}

void CameraLoadViewPoseFromCar(GameViewWork *view,
                                const GameCarRuntime *car) {
    CameraLoadViewPositionFromCar(view, car);
    view->angleX = car->bodyPitch;
    view->angleY = car->bodyYaw;
    view->angleZ = car->bodyRoll;
    view->depth = car->bodyRotationW;
}

void CameraBuildCarRotation(Matrix *rotation, const GameCarRuntime *car) {
    Matrix axisRotation;

    BuildRotMatrixY(rotation, car->bodyYaw);
    BuildRotMatrixX(&axisRotation, car->bodyPitch);
    MulMatrix2(&axisRotation, rotation);
    BuildRotMatrixZ(&axisRotation, car->bodyRoll);
    MulMatrix2(&axisRotation, rotation);
}
