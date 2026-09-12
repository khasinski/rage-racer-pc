#include "game/angle.h"
#include "camera_internal.h"

/* Mode-3 nodes store orientation in the first four data words where modes
 * 2 and 4 store a world position; the record is a union keyed by node mode. */
static s32 ShortestAngleDelta(s32 delta) {
    if (delta >= 0x800)
        return CameraSubtractWord(delta, 0x1000);
    if (delta < -0x7FF)
        return CameraAddWord(delta, 0x1000);
    return delta;
}

static s32 InterpolateCameraValue(s32 start, s32 delta, s32 blend) {
    s32 product = CameraMultiplyWord(blend, delta);

    if (product < 0) {
        product = CameraAddWord(product, 0x1FFF);
    }
    return CameraAddWord(start, product >> 13);
}

static s32 BlendCameraCoordinate(s32 current, s32 target, s32 blend,
                                 s32 scale) {
    s32 distance = CameraSubtractWord(current, target);
    s32 adjustment = CameraMultiplyWord(distance, blend) / scale;

    return CameraSubtractWord(current, adjustment);
}

static s32 MoveCameraCoordinate(s32 current, s32 target, s32 frame,
                                s32 duration) {
    s32 distance = CameraSubtractWord(target, current);
    s32 movement = CameraMultiplyWord(distance, frame) / duration;

    return CameraAddWord(current, movement);
}

static s32 CameraNodeDuration(const GameTrackCameraNode *node) {
    return node->duration > 0 ? node->duration : 1;
}

/*
 * Point the camera at a place in the world: pitch and yaw from the camera to
 * the target, in the game's 0x1000-per-turn angle units, with no roll.
 */
static void AimCameraAt(GameViewWork *view, s32 targetX, s32 targetY, s32 targetZ) {
    s32 dx = CameraSubtractWord(view->x, targetX);
    s32 dy = CameraSubtractWord(view->y, targetY);
    s32 dz = CameraSubtractWord(view->z, targetZ);
    s32 horizontalDistanceSquared = CameraAddWord(
        CameraMultiplyWord(dx, dx), CameraMultiplyWord(dz, dz));

    view->angleX = 0x400 -
        (Atan2(CameraSubtractWord(0, dy),
               SquareRoot0(horizontalDistanceSquared)) & ANGLE_MASK);
    view->angleY = 0x400 -
        (Atan2(CameraSubtractWord(0, dx),
               CameraSubtractWord(0, dz)) & ANGLE_MASK);
    view->angleZ = 0;
}

/*
 * Mode 2: a camera watching the car from a fixed spot beside the track,
 * dragged towards it by the node's own blend.
 */
void CameraViewFromBlendedNode(Camera *camera, GameCarRuntime *car, GameViewWork *view,
                                s32 cameraNodeIndex) {
    s32 blend;
    const GameTrackCameraNode *chaseNode;
    s32 focusX;
    s32 focusY;
    s32 focusZ;
    Matrix inverseObjectRotation;
    Vec4 nodeOffset = {0};
    Vec4 nodeWorld = {0};
    Matrix objectRotation;

    chaseNode = &g_TrackCameras[cameraNodeIndex];
    view->x = chaseNode->data.world.x;
    view->y = chaseNode->data.world.y;
    view->z = chaseNode->data.world.z;
    view->parameter = chaseNode->data.world.blend;
    CameraBuildCarRotation(&objectRotation, car);
    TransposeMatrix(&objectRotation, &inverseObjectRotation);
    /* The point on the car the node looks at, in the car's frame and
     * then in the world. */
    nodeOffset.x = chaseNode->offset[0];
    nodeOffset.y = chaseNode->offset[1];
    nodeOffset.z = CameraAddWord(chaseNode->offset[2], 0x32);
    ApplyMatrixLV(&inverseObjectRotation, AsWords(&nodeOffset),
                  AsWords(&nodeWorld));
    focusX = CameraAddWord(car->x, nodeWorld.x);
    focusY = CameraAddWord(car->y, nodeWorld.y);
    focusZ = CameraAddWord(car->z, nodeWorld.z);
    /* Pull the node's camera towards that point by the node's own blend,
     * then aim from where it ended up. */
    blend = chaseNode->data.world.blend;
    view->x = BlendCameraCoordinate(view->x, focusX, blend, 10000);
    view->y = BlendCameraCoordinate(view->y, focusY, blend, 10000);
    view->z = BlendCameraCoordinate(view->z, focusZ, blend, 10000);
    AimCameraAt(view, focusX, focusY, focusZ);
    camera->previousMode = TRACK_CAMERA_BLENDED_NODE;
}

/*
 * Mode 3: the scripted camera path. Offset and orientation are both eased
 * from one node to the next across the node's duration, and the roll comes
 * off the finished view rather than off the car.
 */
void CameraViewFromCamPath(Camera *camera, GameCarRuntime *car, GameViewWork *view,
                            s32 cameraNodeIndex, int nodeChanged) {
    s32 camPathAngle;
    s32 camPathOffset;
    Matrix cameraRotation;
    Vec4 eyeOffset = {0};
    Vec4 eyeWorld = {0};
    Vec4 focusOffset = {0};
    Vec4 focusWorld = {0};
    s32 focusX;
    s32 focusY;
    s32 focusZ;
    Matrix inverseObjectRotation;
    Matrix matrixWork;
    Matrix objectRotation;
    s32 pathBlend;
    const GameTrackCameraNode *pathNode;
    s32 pathOffsetY;
    s32 pathOffsetZ;
    s32 pathPitch;
    s32 pathRoll;
    s32 pathYaw;
    s32 pathYawRelative;
    s32 pitchDelta;
    s32 duration;
    const GameTrackCameraNode *prevNode;
    Vec4 rollProbe = {0};
    Vec4 rollWork = {0};

    CameraLoadViewPositionFromCar(view, car);
    if (nodeChanged || camera->previousMode != TRACK_CAMERA_PATH) {
        camera->path.node = cameraNodeIndex;
        camera->path.frame = 0;
        if (camera->previousMode == TRACK_CAMERA_PATH) {
            camera->path.offsetStart[0] = camera->path.offset[0];
            camera->path.offsetStart[1] = camera->path.offset[1];
            camera->path.offsetStart[2] = camera->path.offset[2];
            camera->path.angleStart[CAMPATH_PITCH] = camera->path.angle[CAMPATH_PITCH];
            camera->path.angleStart[CAMPATH_YAW] = camera->path.angle[CAMPATH_YAW];
            camera->path.angleStart[CAMPATH_ROLL] = camera->path.angle[CAMPATH_ROLL];
            camera->path.angleStart[CAMPATH_DIST] = camera->path.angle[CAMPATH_DIST];
        } else {
            prevNode = &g_TrackCameras[cameraNodeIndex];
            camera->path.offsetStart[0] = prevNode->offset[0];
            camera->path.offsetStart[1] = prevNode->offset[1];
            camera->path.offsetStart[2] = prevNode->offset[2];
            camera->path.angleStart[CAMPATH_PITCH] = prevNode->data.orientation.pitch;
            camera->path.angleStart[CAMPATH_YAW] = prevNode->data.orientation.yaw;
            camera->path.angleStart[CAMPATH_ROLL] = prevNode->data.orientation.roll;
            camera->path.angleStart[CAMPATH_DIST] = prevNode->data.orientation.distance;
        }
        pathNode = &g_TrackCameras[camera->path.node];
        camera->path.offsetDelta[0] = CameraSubtractWord(
            pathNode->offset[0], camera->path.offsetStart[0]);
        camera->path.offsetDelta[1] = CameraSubtractWord(
            pathNode->offset[1], camera->path.offsetStart[1]);
        camera->path.offsetDelta[2] = CameraSubtractWord(
            pathNode->offset[2], camera->path.offsetStart[2]);
        pitchDelta = CameraSubtractWord(
            pathNode->data.orientation.pitch,
            camera->path.angleStart[CAMPATH_PITCH]);
        camera->path.angleDelta[CAMPATH_PITCH] =
            ShortestAngleDelta(pitchDelta);
        camera->path.angleDelta[CAMPATH_YAW] = CameraSubtractWord(
            pathNode->data.orientation.yaw,
            camera->path.angleStart[CAMPATH_YAW]);
        camera->path.angleDelta[CAMPATH_ROLL] = CameraSubtractWord(
            pathNode->data.orientation.roll,
            camera->path.angleStart[CAMPATH_ROLL]);
        camera->path.angleDelta[CAMPATH_DIST] = CameraSubtractWord(
            pathNode->data.orientation.distance,
            camera->path.angleStart[CAMPATH_DIST]);
        camera->path.angleDelta[CAMPATH_YAW] = ShortestAngleDelta(
            camera->path.angleDelta[CAMPATH_YAW]);
        camera->path.angleDelta[CAMPATH_ROLL] = ShortestAngleDelta(
            camera->path.angleDelta[CAMPATH_ROLL]);
    } else if (camera->path.frame <
               CameraNodeDuration(&g_TrackCameras[camera->path.node])) {
        camera->path.frame += 1;
    }
    duration = CameraNodeDuration(&g_TrackCameras[camera->path.node]);
    pathBlend = 0x1000 - rcos((s32)(
        (int64_t)camera->path.frame * 0x800 / duration));
    camPathOffset = InterpolateCameraValue(
        camera->path.offsetStart[0], camera->path.offsetDelta[0], pathBlend);
    focusOffset.x = camPathOffset;
    pathOffsetY = InterpolateCameraValue(
        camera->path.offsetStart[1], camera->path.offsetDelta[1], pathBlend);
    focusOffset.y = pathOffsetY;
    pathOffsetZ = InterpolateCameraValue(
        camera->path.offsetStart[2], camera->path.offsetDelta[2], pathBlend);
    focusOffset.z = pathOffsetZ;
    pathPitch = InterpolateCameraValue(camera->path.angleStart[CAMPATH_PITCH],
                                       camera->path.angleDelta[CAMPATH_PITCH],
                                       pathBlend);
    pathYaw = InterpolateCameraValue(camera->path.angleStart[CAMPATH_YAW],
                                     camera->path.angleDelta[CAMPATH_YAW],
                                     pathBlend);
    pathRoll = InterpolateCameraValue(camera->path.angleStart[CAMPATH_ROLL],
                                      camera->path.angleDelta[CAMPATH_ROLL],
                                      pathBlend);
    camera->path.angle[CAMPATH_PITCH] = pathPitch & ANGLE_MASK;
    camera->path.angle[CAMPATH_YAW] = pathYaw & ANGLE_MASK;
    camera->path.angle[CAMPATH_ROLL] = pathRoll & ANGLE_MASK;
    camera->path.offset[0] = camPathOffset;
    camera->path.offset[1] = pathOffsetY;
    camera->path.offset[2] = pathOffsetZ;
    camPathAngle = InterpolateCameraValue(
        camera->path.angleStart[CAMPATH_DIST],
        camera->path.angleDelta[CAMPATH_DIST], pathBlend);
    camera->path.angle[CAMPATH_DIST] = camPathAngle;
    pathYawRelative = CameraSubtractWord(pathYaw, car->bodyYaw);
    BuildRotMatrixY(&cameraRotation, pathYawRelative);
    BuildRotMatrixX(&matrixWork, pathPitch);
    MulMatrix2(&matrixWork, &cameraRotation);
    BuildRotMatrixZ(&matrixWork, pathRoll);
    MulMatrix2(&matrixWork, &cameraRotation);
    CameraBuildCarRotation(&objectRotation, car);
    TransposeMatrix(&objectRotation, &inverseObjectRotation);
    MulMatrix2(&cameraRotation, &objectRotation);
    TransposeMatrix(&objectRotation, &matrixWork);
    focusOffset.z = CameraAddWord(focusOffset.z, 0x32);
    ApplyMatrixLV(&inverseObjectRotation, AsWords(&focusOffset),
                  AsWords(&focusWorld));
    focusX = CameraAddWord(view->x, focusWorld.x);
    focusY = CameraAddWord(view->y, focusWorld.y);
    focusZ = CameraAddWord(view->z, focusWorld.z);
    /* Sit the path's distance behind the focus point, then look back at
     * it. */
    eyeOffset.z = camera->path.angle[CAMPATH_DIST];
    ApplyMatrixLV(&matrixWork, AsWords(&eyeOffset), AsWords(&eyeWorld));
    view->x = CameraSubtractWord(focusX, eyeWorld.x);
    view->y = CameraSubtractWord(focusY, eyeWorld.y);
    view->z = CameraSubtractWord(focusZ, eyeWorld.z);
    AimCameraAt(view, focusX, focusY, focusZ);
    /* Roll: take the camera's own right-hand axis back through the view
     * matrix and read how far off level it lands. */
    rollProbe.x = 0x1000;
    BuildRotMatrixY(&cameraRotation,
                    CameraSubtractWord(0, view->angleY));
    ApplyMatrixLV(&cameraRotation, AsWords(&rollProbe), AsWords(&rollWork));
    TransposeMatrix(&matrixWork, &cameraRotation);
    ApplyMatrixLV(&cameraRotation, AsWords(&rollWork), AsWords(&rollProbe));
    view->angleZ = 0x400 - (Atan2(rollProbe.y, rollProbe.x) & ANGLE_MASK);
    camera->previousMode = TRACK_CAMERA_PATH;
}

/*
 * Mode 4: a node that slides to its own position across its duration, then
 * looks back at the car.
 */
void CameraViewFromSlidingNode(Camera *camera, GameCarRuntime *car, GameViewWork *view,
                                s32 cameraNodeIndex, int nodeChanged) {
    Matrix inverseObjectRotation;
    Vec4 nodeOffset = {0};
    Vec4 nodeWorld = {0};
    Matrix objectRotation;
    const GameTrackCameraNode *orbitNode;
    s32 duration;

    orbitNode = &g_TrackCameras[cameraNodeIndex];
    view->x = orbitNode->data.world.x;
    view->y = orbitNode->data.world.y;
    view->z = orbitNode->data.world.z;
    view->parameter = orbitNode->data.orientation.distance;
    if (nodeChanged || camera->previousMode != TRACK_CAMERA_SLIDING_NODE) {
        camera->path.frame = 0;
    } else if (camera->path.frame <
               CameraNodeDuration(&g_TrackCameras[cameraNodeIndex])) {
        camera->path.frame += 1;
    }
    CameraBuildCarRotation(&objectRotation, car);
    TransposeMatrix(&objectRotation, &inverseObjectRotation);
    duration = CameraNodeDuration(orbitNode);
    nodeOffset.y = orbitNode->data.orientation.distance;
    nodeOffset.z = 0x32;
    ApplyMatrixLV(&inverseObjectRotation, AsWords(&nodeOffset),
                  AsWords(&nodeWorld));
    /* Slide the camera from where it starts to the node's own position
     * across the node's duration, then aim back at the car. */
    view->x = MoveCameraCoordinate(
        view->x, orbitNode->offset[0], camera->path.frame, duration);
    view->y = MoveCameraCoordinate(
        view->y, orbitNode->offset[1], camera->path.frame, duration);
    view->z = MoveCameraCoordinate(
        view->z, orbitNode->offset[2], camera->path.frame, duration);
    AimCameraAt(view, CameraAddWord(car->x, nodeWorld.x),
                CameraAddWord(car->y, nodeWorld.y),
                CameraAddWord(car->z, nodeWorld.z));
    camera->previousMode = TRACK_CAMERA_SLIDING_NODE;
}
