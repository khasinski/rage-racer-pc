#ifndef RAGE_CAMERA_INTERNAL_H
#define RAGE_CAMERA_INTERNAL_H

#include "game/render.h"
#include "game/track_internal.h"

void CameraLoadViewPositionFromCar(GameViewWork *view,
                                   const GameRenderObject *car);
void CameraLoadViewPoseFromCar(GameViewWork *view,
                               const GameRenderObject *car);
void CameraViewFromCarBlock(GameRenderObject *car, GameViewWork *view);
void CameraViewFromChaseCamera(GameRenderObject *car, GameViewWork *view);
void CameraViewFromBlendedNode(GameRenderObject *car, GameViewWork *view,
                               s32 cameraNodeIndex);
void CameraViewFromCamPath(GameRenderObject *car, GameViewWork *view,
                           s32 cameraNodeIndex, int nodeChanged);
void CameraViewFromSlidingNode(GameRenderObject *car, GameViewWork *view,
                               s32 cameraNodeIndex, int nodeChanged);
void CameraViewFromOrbit(GameRenderObject *car, GameViewWork *view);

#endif
