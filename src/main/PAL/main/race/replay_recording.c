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
    u32 value;

    value = g_GrandPrixMode;
    g_ReplayWriteCursor = 0;
    if (value != 0) {
        value = 0x5DC;
    } else {
        value = 0xA0A;
    }
    g_ReplayFrameCount = value;
    g_ReplayBufferWrapped = 0;
}

void StoreReplayCarFrame(s32 pairIndex, const GameRenderSourcePoint *srcA,
                         const GameRenderSourcePoint *srcB) {
    ReplayGrandPrixFrame *dst;
    const GameRenderSourcePoint *src1;
    const GameRenderSourcePoint *src2;
    s32 sourceModelIndex;
    s32 current;
    s32 odd;
    u32 first;

    current = g_PlayerCar.modelIndex;
    src2 = srcB;
    sourceModelIndex = src2->modelIndex;
    g_ReplayPlayerModel.word = current;
    odd = pairIndex & 1;
    g_ReplayRivalModel.word = sourceModelIndex;
    if (odd) {
        return;
    }

    pairIndex >>= 1;
    dst = &g_ReplayFramesGp[pairIndex];
    src1 = srcA;
    first = src1->x;
    dst->x0 = first;
    dst->y0 = src1->y;
    dst->z0 = src1->z;
    dst->modelY0 = src1->modelY;
    dst->bodyPitch0 = src1->bodyPitch;
    dst->bodyYaw0 = src1->bodyYaw;
    dst->bodyRoll0 = src1->bodyRoll;
    dst->wheelRotation0 = src1->wheelRotation;
    dst->steeringAngle0 = src1->steeringAngle;
    dst->x1 = src2->x;
    dst->y1 = src2->y;
    dst->z1 = src2->z;
    dst->modelY1 = src2->modelY;
    dst->bodyPitch1 = src2->bodyPitch;
    dst->bodyYaw1 = src2->bodyYaw;
    dst->bodyRoll1 = src2->bodyRoll;
    dst->wheelRotation1 = src2->wheelRotation;
    dst->steeringAngle1 = src2->steeringAngle;
    dst->trackPointIndex0 = src1->trackPointIndex;
    dst->trackPointIndex1 = src2->trackPointIndex;
    dst->tiltCounter = src1->tiltCounter;
}

void StoreReplayTimeAttackFrame(s32 pointIndex, const GameRenderSourcePoint *srcPtr) {
    ReplayTimeAttackFrame *dst;
    const GameRenderSourcePoint *src;
    u32 first;

    g_ReplayPlayerModel.word = g_PlayerCar.modelIndex;
    if (pointIndex % 2) {
        return;
    }

    pointIndex >>= 1;
    dst = &g_ReplayFramesTimeAttack[pointIndex];
    src = srcPtr;
    first = src->x;
    dst->x = first;
    dst->y = src->y;
    dst->z = src->z;
    dst->modelY = src->modelY;
    dst->bodyPitch = src->bodyPitch;
    dst->bodyYaw = src->bodyYaw;
    dst->bodyRoll = src->bodyRoll;
    dst->wheelRotation = src->wheelRotation;
    dst->steeringAngle = src->steeringAngle;
    dst->trackPointIndex = src->trackPointIndex;
    dst->tiltCounter = src->tiltCounter;
}
