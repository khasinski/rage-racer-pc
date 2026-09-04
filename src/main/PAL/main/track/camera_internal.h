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
                                   const GameRenderObject *car);
void CameraLoadViewPoseFromCar(GameViewWork *view,
                               const GameRenderObject *car);
void CameraBuildCarRotation(Matrix *rotation, const GameRenderObject *car);
void CameraViewFromCarBlock(GameRenderObject *car, GameViewWork *view);
void CameraViewFromChaseCamera(GameRenderObject *car, GameViewWork *view);
void CameraViewFromBlendedNode(GameRenderObject *car, GameViewWork *view,
                               s32 cameraNodeIndex);
void CameraViewFromCamPath(GameRenderObject *car, GameViewWork *view,
                           s32 cameraNodeIndex, int nodeChanged);
void CameraViewFromSlidingNode(GameRenderObject *car, GameViewWork *view,
                               s32 cameraNodeIndex, int nodeChanged);
void CameraViewFromOrbit(GameRenderObject *car, GameViewWork *view);
void CameraViewFromLookBehind(GameRenderObject *car, GameViewWork *view);

#endif
