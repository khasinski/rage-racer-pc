#include "camera_internal.h"
#include "game/car.h"

#define ASSERT_RENDER_CAR_FIELD(field)                                      \
    _Static_assert(offsetof(GameRenderObject, field) ==                     \
                       offsetof(GameCarRuntime, field),                     \
                   "GameRenderObject prefix has drifted at " #field)

ASSERT_RENDER_CAR_FIELD(positionW);
ASSERT_RENDER_CAR_FIELD(bodyYaw);
ASSERT_RENDER_CAR_FIELD(bodyRotationW);
ASSERT_RENDER_CAR_FIELD(normalizedLateralOffset);
ASSERT_RENDER_CAR_FIELD(reserved40);
ASSERT_RENDER_CAR_FIELD(collisionFlag);
ASSERT_RENDER_CAR_FIELD(acceleration);
ASSERT_RENDER_CAR_FIELD(initializedFlag);
ASSERT_RENDER_CAR_FIELD(aiEnabled);
ASSERT_RENDER_CAR_FIELD(reservedC4);
ASSERT_RENDER_CAR_FIELD(worldVelocityX);
ASSERT_RENDER_CAR_FIELD(reservedCC);
ASSERT_RENDER_CAR_FIELD(worldVelocityZ);

#undef ASSERT_RENDER_CAR_FIELD

void CameraLoadViewPositionFromCar(GameViewWork *view,
                                    const GameRenderObject *car) {
    view->x = car->x;
    view->y = car->y;
    view->z = car->z;
    view->parameter = car->positionW;
}

void CameraLoadViewPoseFromCar(GameViewWork *view,
                                const GameRenderObject *car) {
    CameraLoadViewPositionFromCar(view, car);
    view->angleX = car->bodyPitch;
    view->angleY = car->bodyYaw;
    view->angleZ = car->bodyRoll;
    view->depth = car->bodyRotationW;
}

void CameraBuildCarRotation(Matrix *rotation, const GameRenderObject *car) {
    Matrix axisRotation;

    BuildRotMatrixY(rotation, car->bodyYaw);
    BuildRotMatrixX(&axisRotation, car->bodyPitch);
    MulMatrix2(&axisRotation, rotation);
    BuildRotMatrixZ(&axisRotation, car->bodyRoll);
    MulMatrix2(&axisRotation, rotation);
}
