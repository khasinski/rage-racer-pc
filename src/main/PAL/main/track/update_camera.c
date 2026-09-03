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
    view->x = CameraAddWord(view->x, cameraLiftWorld[0] >> 4);
    view->y = CameraAddWord(view->y, cameraLiftWorld[1] >> 4);
    view->z = CameraAddWord(view->z, cameraLiftWorld[2] >> 4);
    view->angleX = CameraAddWord(view->angleX, car->tiltCounter);
    g_CameraModePrev = TRACK_CAMERA_CAR;
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
    Matrix cameraToWorld;
    Matrix objectRotation;
    CameraLoadViewPositionFromCar(view, car);
    BuildRotMatrixY(&cameraRotation,
                    CameraSubtractWord(0, g_OrbitCameraYaw));
    CameraBuildCarRotation(&objectRotation, car);
    TransposeMatrix(&objectRotation, &inverseObjectRotation);
    MulMatrix2(&cameraRotation, &objectRotation);
    TransposeMatrix(&objectRotation, &cameraToWorld);
    focusOffset[0] = 0;
    focusOffset[1] = 0;
    focusOffset[2] = 0x32;
    ApplyMatrixLV(&inverseObjectRotation, &focusOffset[0],
                  &focusWorld[0]);
    view->x = CameraAddWord(view->x, focusWorld[0]);
    view->y = CameraAddWord(view->y, focusWorld[1]);
    view->z = CameraAddWord(view->z, focusWorld[2]);
    eyeOffset[0] = 0;
    eyeOffset[1] = 0;
    eyeOffset[2] = g_OrbitCameraDistance;
    ApplyMatrixLV(&cameraToWorld, &eyeOffset[0], &eyeWorld[0]);
    /* Pitch uses the orbit distance rather than the flattened eye vector,
     * so a pitched camera tilts a shade less than a true look-at would.
     * Retail's, and the view players know. */
    view->angleX = 0x400 - (Atan2(eyeWorld[1], g_OrbitCameraDistance) & 0xFFF);
    view->angleY = 0x400 - (Atan2(eyeWorld[0], eyeWorld[2]) & 0xFFF);
    view->angleZ = car->bodyRoll;
    g_CameraModePrev = TRACK_CAMERA_ORBIT;
    view->x = CameraSubtractWord(view->x, eyeWorld[0]);
    view->y = CameraSubtractWord(
        CameraSubtractWord(view->y, 0x28), eyeWorld[1]);
    view->z = CameraSubtractWord(view->z, eyeWorld[2]);
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
    if (cameraModeSel < CAMERA_VIEW_TRACK) {
        cameraMode = cameraModeSel;
    } else if (cameraNodeIndex >= 0) {
        cameraMode = g_TrackCameras[cameraNodeIndex].mode;
    } else {
        cameraMode = 0;
    }
    switch (cameraMode) {
    default:
    case TRACK_CAMERA_CAR:
        CameraViewFromCarBlock(car, view);
        break;
    case TRACK_CAMERA_CHASE:
        CameraViewFromChaseCamera(car, view);
        break;
    case TRACK_CAMERA_BLENDED_NODE:
        CameraViewFromBlendedNode(car, view, cameraNodeIndex);
        break;
    case TRACK_CAMERA_PATH:
        CameraViewFromCamPath(car, view, cameraNodeIndex, nodeChanged);
        break;
    case TRACK_CAMERA_SLIDING_NODE:
        CameraViewFromSlidingNode(car, view, cameraNodeIndex, nodeChanged);
        break;
    case TRACK_CAMERA_ORBIT:
        CameraViewFromOrbit(car, view);
        break;
    }
    StoreViewWork(&viewWork);
    SetCameraRotMatrix();
    if (cameraModeSel > 0 &&
        car == GetCarRenderObject(AsRivalCar(&g_PlayerCar))) {
        SelectModelBank(0);
        DrawPlayerCarModel(car);
    }
}
