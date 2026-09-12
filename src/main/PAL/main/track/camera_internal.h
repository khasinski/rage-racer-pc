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
void CameraViewFromCarBlock(GameCarRuntime *car, GameViewWork *view);
void CameraViewFromChaseCamera(GameCarRuntime *car, GameViewWork *view);
void CameraViewFromBlendedNode(GameCarRuntime *car, GameViewWork *view,
                               s32 cameraNodeIndex);
void CameraViewFromCamPath(GameCarRuntime *car, GameViewWork *view,
                           s32 cameraNodeIndex, int nodeChanged);
void CameraViewFromSlidingNode(GameCarRuntime *car, GameViewWork *view,
                               s32 cameraNodeIndex, int nodeChanged);
void CameraViewFromOrbit(GameCarRuntime *car, GameViewWork *view);
void CameraViewFromLookBehind(GameCarRuntime *car, GameViewWork *view);

#endif
