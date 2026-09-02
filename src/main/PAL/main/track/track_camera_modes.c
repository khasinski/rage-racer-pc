#include "camera_internal.h"

/* Mode-3 nodes store orientation in the first four data words where modes
 * 2 and 4 store a world position; the record is a union keyed by node mode. */
static s32 ShortestAngleDelta(s32 delta) {
    if (delta >= 0x800)
        return delta - 0x1000;
    if (delta < -0x7FF)
        return delta + 0x1000;
    return delta;
}

static s32 InterpolateCameraValue(s32 start, s32 delta, s32 blend) {
    s32 product = blend * delta;

    if (product < 0) {
        product += 0x1FFF;
    }
    return start + (product >> 13);
}

static s32 CameraNodeDuration(const GameTrackCameraNode *node) {
    return node->duration > 0 ? node->duration : 1;
}

/*
 * Point the camera at a place in the world: pitch and yaw from the camera to
 * the target, in the game's 0x1000-per-turn angle units, with no roll.
 */
static void AimCameraAt(GameViewWork *view, s32 targetX, s32 targetY, s32 targetZ) {
    s32 dx = view->x - targetX;
    s32 dy = view->y - targetY;
    s32 dz = view->z - targetZ;
    view->angleX = 0x400 - (Atan2(-dy, SquareRoot0(dx * dx + dz * dz)) & 0xFFF);
    view->angleY = 0x400 - (Atan2(-dx, -dz) & 0xFFF);
    view->angleZ = 0;
}

/*
 * Mode 2: a camera watching the car from a fixed spot beside the track,
 * dragged towards it by the node's own blend.
 */
void CameraViewFromBlendedNode(GameRenderObject *car, GameViewWork *view,
                                s32 cameraNodeIndex) {
    s32 blend;
    GameTrackCameraNode *chaseNode;
    s32 focusX;
    s32 focusY;
    s32 focusZ;
    Matrix inverseObjectRotation;
    Matrix matrixWork;
    s32 nodeOffset[3];
    s32 nodeWorld[3];
    Matrix objectRotation;

    chaseNode = &g_TrackCameras[cameraNodeIndex];
    view->x = chaseNode->data.world.x;
    view->y = chaseNode->data.world.y;
    view->z = chaseNode->data.world.z;
    view->reserved = chaseNode->data.world.blend;
    BuildRotMatrixY(&objectRotation, car->bodyYaw);
    BuildRotMatrixX(&matrixWork, car->bodyPitch);
    MulMatrix2(&matrixWork, &objectRotation);
    BuildRotMatrixZ(&matrixWork, car->bodyRoll);
    MulMatrix2(&matrixWork, &objectRotation);
    TransposeMatrix(&objectRotation, &inverseObjectRotation);
    /* The point on the car the node looks at, in the car's frame and
     * then in the world. */
    nodeOffset[0] = chaseNode->offset[0];
    nodeOffset[1] = chaseNode->offset[1];
    nodeOffset[2] = chaseNode->offset[2] + 0x32;
    ApplyMatrixLV(&inverseObjectRotation, &nodeOffset[0],
                  &nodeWorld[0]);
    focusX = car->x + nodeWorld[0];
    focusY = car->y + nodeWorld[1];
    focusZ = car->z + nodeWorld[2];
    /* Pull the node's camera towards that point by the node's own blend,
     * then aim from where it ended up. */
    blend = chaseNode->data.world.blend;
    view->x -= ((view->x - focusX) * blend) / 10000;
    view->y -= ((view->y - focusY) * blend) / 10000;
    view->z -= ((view->z - focusZ) * blend) / 10000;
    AimCameraAt(view, focusX, focusY, focusZ);
    g_CameraModePrev = TRACK_CAMERA_BLENDED_NODE;
}

/*
 * Mode 3: the scripted camera path. Offset and orientation are both eased
 * from one node to the next across the node's duration, and the roll comes
 * off the finished view rather than off the car.
 */
void CameraViewFromCamPath(GameRenderObject *car, GameViewWork *view,
                            s32 cameraNodeIndex, int nodeChanged) {
    s32 camPathAngle;
    s32 camPathOffset;
    Matrix cameraRotation;
    s32 eyeOffset[3];
    s32 eyeWorld[3];
    s32 focusOffset[3];
    s32 focusWorld[3];
    s32 focusX;
    s32 focusY;
    s32 focusZ;
    Matrix inverseObjectRotation;
    Matrix matrixWork;
    Matrix objectRotation;
    s32 pathBlend;
    GameTrackCameraNode *pathNode;
    s32 pathOffsetY;
    s32 pathOffsetZ;
    s32 pathPitch;
    s32 pathRoll;
    s32 pathYaw;
    s32 pathYawRelative;
    s32 pitchDelta;
    s32 duration;
    GameTrackCameraNode *prevNode;
    s32 rollProbe[3];
    s32 rollWork[3];

    CameraLoadViewPositionFromCar(view, car);
    if (nodeChanged || g_CameraModePrev != TRACK_CAMERA_PATH) {
        g_CamPathNode = cameraNodeIndex;
        g_CamPathFrame = 0;
        if (g_CameraModePrev == TRACK_CAMERA_PATH) {
            g_CamPathOffsetStart[0] = g_CamPathOffset[0];
            g_CamPathOffsetStart[1] = g_CamPathOffset[1];
            g_CamPathOffsetStart[2] = g_CamPathOffset[2];
            g_CamPathAngleStart[CAMPATH_PITCH] = g_CamPathAngle[CAMPATH_PITCH];
            g_CamPathAngleStart[CAMPATH_YAW] = g_CamPathAngle[CAMPATH_YAW];
            g_CamPathAngleStart[CAMPATH_ROLL] = g_CamPathAngle[CAMPATH_ROLL];
            g_CamPathAngleStart[CAMPATH_DIST] = g_CamPathAngle[CAMPATH_DIST];
        } else {
            prevNode = &g_TrackCameras[cameraNodeIndex];
            g_CamPathOffsetStart[0] = prevNode->offset[0];
            g_CamPathOffsetStart[1] = prevNode->offset[1];
            g_CamPathOffsetStart[2] = prevNode->offset[2];
            g_CamPathAngleStart[CAMPATH_PITCH] = prevNode->data.orientation.pitch;
            g_CamPathAngleStart[CAMPATH_YAW] = prevNode->data.orientation.yaw;
            g_CamPathAngleStart[CAMPATH_ROLL] = prevNode->data.orientation.roll;
            g_CamPathAngleStart[CAMPATH_DIST] = prevNode->data.orientation.distance;
        }
        pathNode = &g_TrackCameras[g_CamPathNode];
        g_CamPathOffsetDelta[0] = pathNode->offset[0] - g_CamPathOffsetStart[0];
        g_CamPathOffsetDelta[1] = pathNode->offset[1] - g_CamPathOffsetStart[1];
        g_CamPathOffsetDelta[2] = pathNode->offset[2] - g_CamPathOffsetStart[2];
        pitchDelta = pathNode->data.orientation.pitch - g_CamPathAngleStart[CAMPATH_PITCH];
        g_CamPathAngleDelta[CAMPATH_PITCH] =
            ShortestAngleDelta(pitchDelta);
        g_CamPathAngleDelta[CAMPATH_YAW] = pathNode->data.orientation.yaw - g_CamPathAngleStart[CAMPATH_YAW];
        g_CamPathAngleDelta[CAMPATH_ROLL] = pathNode->data.orientation.roll - g_CamPathAngleStart[CAMPATH_ROLL];
        g_CamPathAngleDelta[CAMPATH_DIST] = pathNode->data.orientation.distance - g_CamPathAngleStart[CAMPATH_DIST];
        g_CamPathAngleDelta[CAMPATH_YAW] = ShortestAngleDelta(
            g_CamPathAngleDelta[CAMPATH_YAW]);
        g_CamPathAngleDelta[CAMPATH_ROLL] = ShortestAngleDelta(
            g_CamPathAngleDelta[CAMPATH_ROLL]);
    } else if (g_CamPathFrame <
               CameraNodeDuration(&g_TrackCameras[g_CamPathNode])) {
        g_CamPathFrame += 1;
    }
    duration = CameraNodeDuration(&g_TrackCameras[g_CamPathNode]);
    pathBlend = 0x1000 - rcos((g_CamPathFrame << 0xB) / duration);
    camPathOffset = InterpolateCameraValue(
        g_CamPathOffsetStart[0], g_CamPathOffsetDelta[0], pathBlend);
    focusOffset[0] = camPathOffset;
    pathOffsetY = InterpolateCameraValue(
        g_CamPathOffsetStart[1], g_CamPathOffsetDelta[1], pathBlend);
    focusOffset[1] = pathOffsetY;
    pathOffsetZ = InterpolateCameraValue(
        g_CamPathOffsetStart[2], g_CamPathOffsetDelta[2], pathBlend);
    focusOffset[2] = pathOffsetZ;
    pathPitch = InterpolateCameraValue(g_CamPathAngleStart[CAMPATH_PITCH],
                                       g_CamPathAngleDelta[CAMPATH_PITCH],
                                       pathBlend);
    pathYaw = InterpolateCameraValue(g_CamPathAngleStart[CAMPATH_YAW],
                                     g_CamPathAngleDelta[CAMPATH_YAW],
                                     pathBlend);
    pathRoll = InterpolateCameraValue(g_CamPathAngleStart[CAMPATH_ROLL],
                                      g_CamPathAngleDelta[CAMPATH_ROLL],
                                      pathBlend);
    g_CamPathAngle[CAMPATH_PITCH] = pathPitch & 0xFFF;
    g_CamPathAngle[CAMPATH_YAW] = pathYaw & 0xFFF;
    g_CamPathAngle[CAMPATH_ROLL] = pathRoll & 0xFFF;
    g_CamPathOffset[0] = camPathOffset;
    g_CamPathOffset[1] = pathOffsetY;
    g_CamPathOffset[2] = pathOffsetZ;
    camPathAngle = InterpolateCameraValue(
        g_CamPathAngleStart[CAMPATH_DIST],
        g_CamPathAngleDelta[CAMPATH_DIST], pathBlend);
    g_CamPathAngle[CAMPATH_DIST] = camPathAngle;
    pathYawRelative = pathYaw - car->bodyYaw;
    BuildRotMatrixY(&cameraRotation, pathYawRelative);
    BuildRotMatrixX(&matrixWork, pathPitch);
    MulMatrix2(&matrixWork, &cameraRotation);
    BuildRotMatrixZ(&matrixWork, pathRoll);
    MulMatrix2(&matrixWork, &cameraRotation);
    BuildRotMatrixY(&objectRotation, car->bodyYaw);
    BuildRotMatrixX(&matrixWork, car->bodyPitch);
    MulMatrix2(&matrixWork, &objectRotation);
    BuildRotMatrixZ(&matrixWork, car->bodyRoll);
    MulMatrix2(&matrixWork, &objectRotation);
    TransposeMatrix(&objectRotation, &inverseObjectRotation);
    MulMatrix2(&cameraRotation, &objectRotation);
    TransposeMatrix(&objectRotation, &matrixWork);
    focusOffset[2] += 0x32;
    ApplyMatrixLV(&inverseObjectRotation, &focusOffset[0],
                  &focusWorld[0]);
    focusX = view->x + focusWorld[0];
    focusY = view->y + focusWorld[1];
    focusZ = view->z + focusWorld[2];
    /* Sit the path's distance behind the focus point, then look back at
     * it. */
    eyeOffset[0] = 0;
    eyeOffset[1] = 0;
    eyeOffset[2] = g_CamPathAngle[CAMPATH_DIST];
    ApplyMatrixLV(&matrixWork, &eyeOffset[0], &eyeWorld[0]);
    view->x = focusX - eyeWorld[0];
    view->y = focusY - eyeWorld[1];
    view->z = focusZ - eyeWorld[2];
    AimCameraAt(view, focusX, focusY, focusZ);
    /* Roll: take the camera's own right-hand axis back through the view
     * matrix and read how far off level it lands. */
    rollProbe[0] = 0x1000;
    rollProbe[1] = 0;
    rollProbe[2] = 0;
    BuildRotMatrixY(&cameraRotation, 0 - view->angleY);
    ApplyMatrixLV(&cameraRotation, &rollProbe[0], &rollWork[0]);
    TransposeMatrix(&matrixWork, &cameraRotation);
    ApplyMatrixLV(&cameraRotation, &rollWork[0], &rollProbe[0]);
    view->angleZ = 0x400 - (Atan2(rollProbe[1], rollProbe[0]) & 0xFFF);
    g_CameraModePrev = TRACK_CAMERA_PATH;
}

/*
 * Mode 4: a node that slides to its own position across its duration, then
 * looks back at the car.
 */
void CameraViewFromSlidingNode(GameRenderObject *car, GameViewWork *view,
                                s32 cameraNodeIndex, int nodeChanged) {
    Matrix inverseObjectRotation;
    Matrix matrixWork;
    s32 nodeOffset[3];
    s32 nodeWorld[3];
    Matrix objectRotation;
    GameTrackCameraNode *orbitNode;
    s32 duration;

    orbitNode = &g_TrackCameras[cameraNodeIndex];
    view->x = orbitNode->data.world.x;
    view->y = orbitNode->data.world.y;
    view->z = orbitNode->data.world.z;
    view->reserved = orbitNode->data.orientation.distance;
    if (nodeChanged || g_CameraModePrev != TRACK_CAMERA_SLIDING_NODE) {
        g_CamPathFrame = 0;
    } else if (g_CamPathFrame <
               CameraNodeDuration(&g_TrackCameras[cameraNodeIndex])) {
        g_CamPathFrame += 1;
    }
    BuildRotMatrixY(&objectRotation, car->bodyYaw);
    BuildRotMatrixX(&matrixWork, car->bodyPitch);
    MulMatrix2(&matrixWork, &objectRotation);
    BuildRotMatrixZ(&matrixWork, car->bodyRoll);
    MulMatrix2(&matrixWork, &objectRotation);
    TransposeMatrix(&objectRotation, &inverseObjectRotation);
    duration = CameraNodeDuration(orbitNode);
    nodeOffset[0] = 0;
    nodeOffset[1] = orbitNode->data.orientation.distance;
    nodeOffset[2] = 0x32;
    ApplyMatrixLV(&inverseObjectRotation, &nodeOffset[0],
                  &nodeWorld[0]);
    /* Slide the camera from where it starts to the node's own position
     * across the node's duration, then aim back at the car. */
    view->x += ((orbitNode->offset[0] - view->x) * g_CamPathFrame) /
                  duration;
    view->y += ((orbitNode->offset[1] - view->y) * g_CamPathFrame) /
                  duration;
    view->z += ((orbitNode->offset[2] - view->z) * g_CamPathFrame) /
                  duration;
    AimCameraAt(view, car->x + nodeWorld[0], car->y + nodeWorld[1],
                car->z + nodeWorld[2]);
    g_CameraModePrev = TRACK_CAMERA_SLIDING_NODE;
}
