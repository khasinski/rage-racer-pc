#ifndef RAGE_CAMERA_INTERNAL_H
#define RAGE_CAMERA_INTERNAL_H

#include "game/render.h"
#include "game/track_internal.h"

static inline s32 CameraAddWord(s32 left, s32 right) {
    return WrapSigned32((int64_t)left + right);
}

static inline s32 CameraSubtractWord(s32 left, s32 right) {
    return WrapSigned32((int64_t)left - right);
}

static inline s32 CameraMultiplyWord(s32 left, s32 right) {
    return WrapSigned32((int64_t)left * right);
}

void CameraLoadViewPositionFromCar(GameViewWork *view,
                                   const GameCarRuntime *car);
void CameraLoadViewPoseFromCar(GameViewWork *view,
                               const GameCarRuntime *car);
void CameraBuildCarRotation(Matrix *rotation, const GameCarRuntime *car);
void CameraViewFromCarBlock(Camera *camera, GameCarRuntime *car, GameViewWork *view);
void CameraViewFromChaseCamera(Camera *camera, GameCarRuntime *car, GameViewWork *view);
void CameraViewFromBlendedNode(Camera *camera, GameCarRuntime *car, GameViewWork *view,
                               s32 cameraNodeIndex);
void CameraViewFromCamPath(Camera *camera, GameCarRuntime *car, GameViewWork *view,
                           s32 cameraNodeIndex, int nodeChanged);
void CameraViewFromSlidingNode(Camera *camera, GameCarRuntime *car, GameViewWork *view,
                               s32 cameraNodeIndex, int nodeChanged);
void CameraViewFromOrbit(Camera *camera, GameCarRuntime *car, GameViewWork *view);
void CameraViewFromLookBehind(Camera *camera, GameCarRuntime *car, GameViewWork *view);

#endif
