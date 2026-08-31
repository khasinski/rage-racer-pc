#include "game/race.h"
#include "game/player_car_internal.h"
#include "game/render.h"
#include "game/replay_internal.h"
#include "game/work_buffer.h"

void ResetReplayFrameCounts(void) {
    g_ReplayFramesGp = g_ReplayFrameBuffer.grandPrixReplay;
    g_ReplayFramesTimeAttack = g_ReplayFrameBuffer.timeAttackReplay;
}

void ResetReplayWriteCursor(void) {
    g_ReplayWriteCursor = 0;
    g_ReplayFrameCount = g_GrandPrixMode != 0 ? 0x5DC : 0xA0A;
    g_ReplayBufferWrapped = 0;
}

void StoreReplayCarFrame(s32 pairIndex, const GameRenderSourcePoint *srcA,
                         const GameRenderSourcePoint *srcB) {
    ReplayGrandPrixFrame *dst;

    g_ReplayPlayerModel.word = g_PlayerCar.modelIndex;
    g_ReplayRivalModel.word = srcB->modelIndex;
    if (pairIndex & 1) {
        return;
    }

    pairIndex >>= 1;
    dst = &g_ReplayFramesGp[pairIndex];
    dst->x0 = srcA->x;
    dst->y0 = srcA->y;
    dst->z0 = srcA->z;
    dst->modelY0 = srcA->modelY;
    dst->bodyPitch0 = srcA->bodyPitch;
    dst->bodyYaw0 = srcA->bodyYaw;
    dst->bodyRoll0 = srcA->bodyRoll;
    dst->wheelRotation0 = srcA->wheelRotation;
    dst->steeringAngle0 = srcA->steeringAngle;
    dst->x1 = srcB->x;
    dst->y1 = srcB->y;
    dst->z1 = srcB->z;
    dst->modelY1 = srcB->modelY;
    dst->bodyPitch1 = srcB->bodyPitch;
    dst->bodyYaw1 = srcB->bodyYaw;
    dst->bodyRoll1 = srcB->bodyRoll;
    dst->wheelRotation1 = srcB->wheelRotation;
    dst->steeringAngle1 = srcB->steeringAngle;
    dst->trackPointIndex0 = srcA->trackPointIndex;
    dst->trackPointIndex1 = srcB->trackPointIndex;
    dst->tiltCounter = srcA->tiltCounter;
}

void StoreReplayTimeAttackFrame(s32 pointIndex, const GameRenderSourcePoint *srcPtr) {
    ReplayTimeAttackFrame *dst;

    g_ReplayPlayerModel.word = g_PlayerCar.modelIndex;
    if (pointIndex % 2) {
        return;
    }

    pointIndex >>= 1;
    dst = &g_ReplayFramesTimeAttack[pointIndex];
    dst->x = srcPtr->x;
    dst->y = srcPtr->y;
    dst->z = srcPtr->z;
    dst->modelY = srcPtr->modelY;
    dst->bodyPitch = srcPtr->bodyPitch;
    dst->bodyYaw = srcPtr->bodyYaw;
    dst->bodyRoll = srcPtr->bodyRoll;
    dst->wheelRotation = srcPtr->wheelRotation;
    dst->steeringAngle = srcPtr->steeringAngle;
    dst->trackPointIndex = srcPtr->trackPointIndex;
    dst->tiltCounter = srcPtr->tiltCounter;
}
