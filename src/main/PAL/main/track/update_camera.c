#include "game/angle.h"
#include "camera_internal.h"
#include "game/player_car_internal.h"

/*
 * Mode 0: the camera sits where the car's own block says, lifted a fixed
 * amount along the car's up axis.
 */
void CameraViewFromCarBlock(Camera *camera, GameCarRuntime *car, GameViewWork *view) {
    SVec cameraLift = {0, -0x1C0, 0, 0};
    LVec cameraLiftWorld;
    Matrix matrixWork;
    Matrix objectRotation;

    CameraLoadViewPoseFromCar(view, car);
    CameraBuildCarRotation(&objectRotation, car);
    TransposeMatrix(&objectRotation, &matrixWork);
    ApplyMatrix(&matrixWork, &cameraLift, &cameraLiftWorld);
    view->x = CameraAddWord(view->x, cameraLiftWorld.x >> 4);
    view->y = CameraAddWord(view->y, cameraLiftWorld.y >> 4);
    view->z = CameraAddWord(view->z, cameraLiftWorld.z >> 4);
    view->angleX = CameraAddWord(view->angleX, car->tiltCounter);
    camera->previousMode = TRACK_CAMERA_CAR;
}

static void CameraViewFromOrbitPosition(Camera *camera, GameCarRuntime *car,
                                        GameViewWork *view, s32 yaw,
                                        s32 distance, s32 height) {
    Matrix cameraRotation;
    Vec4 eyeOffset = {0};
    Vec4 eyeWorld = {0};
    Vec4 focusOffset = {0};
    Vec4 focusWorld = {0};
    Matrix inverseObjectRotation;
    Matrix cameraToWorld;
    Matrix objectRotation;
    CameraLoadViewPositionFromCar(view, car);
    BuildRotMatrixY(&cameraRotation,
                    CameraSubtractWord(0, yaw));
    CameraBuildCarRotation(&objectRotation, car);
    TransposeMatrix(&objectRotation, &inverseObjectRotation);
    MulMatrix2(&cameraRotation, &objectRotation);
    TransposeMatrix(&objectRotation, &cameraToWorld);
    focusOffset.z = 0x32;
    ApplyMatrixLV(&inverseObjectRotation, AsWords(&focusOffset),
                  AsWords(&focusWorld));
    view->x = CameraAddWord(view->x, focusWorld.x);
    view->y = CameraAddWord(view->y, focusWorld.y);
    view->z = CameraAddWord(view->z, focusWorld.z);
    eyeOffset.y = height;
    eyeOffset.z = distance;
    ApplyMatrixLV(&cameraToWorld, AsWords(&eyeOffset), AsWords(&eyeWorld));
    /* Pitch uses the orbit distance rather than the flattened eye vector,
     * so a pitched camera tilts a shade less than a true look-at would.
     * Retail's, and the view players know. */
    view->angleX = 0x400 - (Atan2(eyeWorld.y, distance) & ANGLE_MASK);
    view->angleY = 0x400 - (Atan2(eyeWorld.x, eyeWorld.z) & ANGLE_MASK);
    view->angleZ = car->bodyRoll;
    camera->previousMode = TRACK_CAMERA_ORBIT;
    view->x = CameraSubtractWord(view->x, eyeWorld.x);
    view->y = CameraSubtractWord(
        CameraSubtractWord(view->y, 0x28), eyeWorld.y);
    view->z = CameraSubtractWord(view->z, eyeWorld.z);
}

void CameraViewFromOrbit(Camera *camera, GameCarRuntime *car, GameViewWork *view) {
    CameraViewFromOrbitPosition(camera, car, view, camera->orbitYaw,
                                camera->orbitDistance, 0);
}

void CameraViewFromLookBehind(Camera *camera, GameCarRuntime *car, GameViewWork *view) {
    enum {
        LOOK_BEHIND_YAW = 0x800,
        LOOK_BEHIND_DISTANCE = 0xE0,
        LOOK_BEHIND_HEIGHT = 0x50,
    };
    CameraViewFromOrbitPosition(camera, car, view, LOOK_BEHIND_YAW,
                                LOOK_BEHIND_DISTANCE, LOOK_BEHIND_HEIGHT);
}

void UpdateCamera(Camera *camera, CameraViewMode cameraModeSel, GameCarRuntime *car) {
    GameViewWork viewWork;
    GameViewWork *view;
    s32 cameraMode;
    s32 cameraNodeIndex;
    s32 previousNodeIndex;
    u8 nodeChanged;

    cameraNodeIndex = FindNearestTrackCamera(car);
    LoadViewWork(&viewWork, &camera->view);
    view = &viewWork;
    previousNodeIndex = camera->node;
    camera->node = cameraNodeIndex;
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
        CameraViewFromCarBlock(camera, car, view);
        break;
    case TRACK_CAMERA_CHASE:
        CameraViewFromChaseCamera(camera, car, view);
        break;
    case TRACK_CAMERA_BLENDED_NODE:
        CameraViewFromBlendedNode(camera, car, view, cameraNodeIndex);
        break;
    case TRACK_CAMERA_PATH:
        CameraViewFromCamPath(camera, car, view, cameraNodeIndex, nodeChanged);
        break;
    case TRACK_CAMERA_SLIDING_NODE:
        CameraViewFromSlidingNode(camera, car, view, cameraNodeIndex, nodeChanged);
        break;
    case TRACK_CAMERA_ORBIT:
        CameraViewFromOrbit(camera, car, view);
        break;
    }
    StoreViewWork(&camera->view, &viewWork);
    SetCameraRotMatrix(&camera->view);
    if (cameraModeSel > 0 &&
        car == AsRivalCar(&g_PlayerCar)) {
        SelectModelBank(0);
        DrawPlayerCarModel(car);
    }
}

void UpdateLookBehindCamera(Camera *camera, GameCarRuntime *car) {
    GameViewWork viewWork;

    LoadViewWork(&viewWork, &camera->view);
    CameraViewFromLookBehind(camera, car, &viewWork);
    StoreViewWork(&camera->view, &viewWork);
    SetCameraRotMatrix(&camera->view);
    SelectModelBank(0);
    DrawPlayerCarModel(car);
}
