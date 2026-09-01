#include "camera_internal.h"
#include "game/player_car_internal.h"

/*
 * Mode 0: the camera sits where the car's own block says, lifted a fixed
 * amount along the car's up axis.
 */
void CameraViewFromCarBlock(GameRenderObject *car, GameViewWork *view) {
    s16 cameraLift[4];
    s32 cameraLiftWorld[4];
    Matrix matrixWork;
    Matrix objectRotation;
    CameraLoadViewPoseFromCar(view, car);
    BuildRotMatrixY(&objectRotation, view->angleY);
    BuildRotMatrixX(&matrixWork, view->angleX);
    MulMatrix2(&matrixWork, &objectRotation);
    BuildRotMatrixZ(&matrixWork, view->angleZ);
    MulMatrix2(&matrixWork, &objectRotation);
    cameraLift[0] = 0;
    cameraLift[1] = -0x1C0;
    cameraLift[2] = 0;
    TransposeMatrix(&objectRotation, &matrixWork);
    ApplyMatrix(&matrixWork, &cameraLift[0], &cameraLiftWorld[0]);
    view->x += cameraLiftWorld[0] >> 4;
    view->y += cameraLiftWorld[1] >> 4;
    view->z += cameraLiftWorld[2] >> 4;
    view->angleX += car->tiltCounter;
    g_CameraModePrev = 0;
}

/*
 * Mode 5: orbiting behind the car at a set distance.
 */
void CameraViewFromOrbit(GameRenderObject *car, GameViewWork *view) {
    Matrix cameraRotation;
    s32 eyeOffset[3];
    s32 eyeWorld[3];
    s32 focusOffset[3];
    s32 focusWorld[3];
    Matrix inverseObjectRotation;
    Matrix matrixWork;
    Matrix objectRotation;
    CameraLoadViewPositionFromCar(view, car);
    BuildRotMatrixY(&cameraRotation, 0 - g_OrbitCameraYaw);
    BuildRotMatrixY(&objectRotation, car->angleY);
    BuildRotMatrixX(&matrixWork, car->bodyPitch);
    MulMatrix2(&matrixWork, &objectRotation);
    BuildRotMatrixZ(&matrixWork, car->bodyRoll);
    MulMatrix2(&matrixWork, &objectRotation);
    TransposeMatrix(&objectRotation, &inverseObjectRotation);
    MulMatrix2(&cameraRotation, &objectRotation);
    TransposeMatrix(&objectRotation, &matrixWork);
    focusOffset[0] = 0;
    focusOffset[1] = 0;
    focusOffset[2] = 0x32;
    ApplyMatrixLV(&inverseObjectRotation, &focusOffset[0],
                  &focusWorld[0]);
    view->x += focusWorld[0];
    view->y += focusWorld[1];
    view->z += focusWorld[2];
    eyeOffset[0] = 0;
    eyeOffset[1] = 0;
    eyeOffset[2] = g_OrbitCameraDistance;
    ApplyMatrixLV(&matrixWork, &eyeOffset[0], &eyeWorld[0]);
    /* Pitch uses the orbit distance rather than the flattened eye vector,
     * so a pitched camera tilts a shade less than a true look-at would.
     * Retail's, and the view players know. */
    view->angleX = 0x400 - (Atan2(eyeWorld[1], g_OrbitCameraDistance) & 0xFFF);
    view->angleY = 0x400 - (Atan2(eyeWorld[0], eyeWorld[2]) & 0xFFF);
    view->angleZ = car->bodyRoll;
    g_CameraModePrev = 5;
    view->x -= eyeWorld[0];
    view->y = (view->y - 0x28) - eyeWorld[1];
    view->z -= eyeWorld[2];
}

void UpdateCamera(CameraViewMode cameraModeSel, GameRenderObject *car) {
    GameViewWork viewWork;
    GameViewWork *view;
    s32 cameraMode;
    s32 cameraNodeIndex;
    s32 previousNodeIndex;
    u8 nodeChanged;

    cameraNodeIndex = FindNearestTrackCamera(car);
    LoadViewWork(&viewWork);
    view = &viewWork;
    previousNodeIndex = g_CameraNodeIndex;
    g_CameraNodeIndex = cameraNodeIndex;
    nodeChanged = cameraNodeIndex != previousNodeIndex;
    if (cameraModeSel < 2) {
        cameraMode = cameraModeSel;
    } else {
        cameraMode = g_TrackCameras[cameraNodeIndex].mode;
    }
    switch (cameraMode) {
    case 0:
        CameraViewFromCarBlock(car, view);
        break;
    case 1:
        CameraViewFromChaseCamera(car, view);
        break;
    case 2:
        CameraViewFromBlendedNode(car, view, cameraNodeIndex);
        break;
    case 3:
        CameraViewFromCamPath(car, view, cameraNodeIndex, nodeChanged);
        break;
    case 4:
        CameraViewFromSlidingNode(car, view, cameraNodeIndex, nodeChanged);
        break;
    case 5:
        CameraViewFromOrbit(car, view);
        break;
    }
    StoreViewWork(&viewWork);
    SetCameraRotMatrix();
    if (cameraModeSel > 0 && car == (GameRenderObject *)&g_PlayerCar) {
        SelectModelBank(0);
        DrawPlayerCarModel(car);
    }
}
