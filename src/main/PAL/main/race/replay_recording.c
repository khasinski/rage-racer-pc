#include "game/car.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/replay_internal.h"
#include "game/work_buffer.h"

void RecordReplayFrame(void) {
    const GameCarRuntime *player = AsRivalCar(&g_PlayerCar);

    if (g_GrandPrixMode != 0) {
        StoreReplayCarFrame(g_ReplayWriteCursor, player, &g_Cars[0]);
    } else {
        StoreReplayTimeAttackFrame(g_ReplayWriteCursor, player);
    }

    g_ReplayWriteCursor++;
    if (g_ReplayWriteCursor == g_ReplayFrameCount) {
        g_ReplayWriteCursor = 0;
        g_ReplayBufferWrapped = 1;
    }
}

void ResetReplayFrameCounts(void) {
    g_ReplayFramesGp = g_ReplayFrameBuffer.grandPrixReplay;
    g_ReplayFramesTimeAttack = g_ReplayFrameBuffer.timeAttackReplay;
}

void ResetReplayWriteCursor(void) {
    g_ReplayWriteCursor = 0;
    g_ReplayFrameCount = g_GrandPrixMode != 0
                             ? GRAND_PRIX_REPLAY_SUBFRAME_COUNT
                             : TIME_ATTACK_REPLAY_SUBFRAME_COUNT;
    g_ReplayBufferWrapped = 0;
}

void StoreReplayCarFrame(s32 pairIndex, const GameCarRuntime *player,
                         const GameCarRuntime *rival) {
    ReplayGrandPrixFrame *dst;

    g_ReplayPlayerModel.word = player->modelIndex;
    g_ReplayRivalModel.word = rival->modelIndex;
    if (pairIndex & 1) {
        return;
    }

    pairIndex >>= 1;
    dst = &g_ReplayFramesGp[pairIndex];
    dst->x0 = player->x;
    dst->y0 = player->y;
    dst->z0 = player->z;
    dst->modelY0 = player->modelY;
    dst->bodyPitch0 = player->bodyPitch;
    dst->bodyYaw0 = player->bodyYaw;
    dst->bodyRoll0 = player->bodyRoll;
    dst->wheelRotation0 = player->wheelRotation;
    dst->steeringAngle0 = player->steeringAngle;
    dst->x1 = rival->x;
    dst->y1 = rival->y;
    dst->z1 = rival->z;
    dst->modelY1 = rival->modelY;
    dst->bodyPitch1 = rival->bodyPitch;
    dst->bodyYaw1 = rival->bodyYaw;
    dst->bodyRoll1 = rival->bodyRoll;
    dst->wheelRotation1 = rival->wheelRotation;
    dst->steeringAngle1 = rival->steeringAngle;
    dst->trackPointIndex0 = player->trackPointIndex;
    dst->trackPointIndex1 = rival->trackPointIndex;
    dst->tiltCounter = player->tiltCounter;
}

void StoreReplayTimeAttackFrame(s32 pointIndex, const GameCarRuntime *player) {
    ReplayTimeAttackFrame *dst;

    g_ReplayPlayerModel.word = player->modelIndex;
    if (pointIndex % 2) {
        return;
    }

    pointIndex >>= 1;
    dst = &g_ReplayFramesTimeAttack[pointIndex];
    dst->x = player->x;
    dst->y = player->y;
    dst->z = player->z;
    dst->modelY = player->modelY;
    dst->bodyPitch = player->bodyPitch;
    dst->bodyYaw = player->bodyYaw;
    dst->bodyRoll = player->bodyRoll;
    dst->wheelRotation = player->wheelRotation;
    dst->steeringAngle = player->steeringAngle;
    dst->trackPointIndex = player->trackPointIndex;
    dst->tiltCounter = player->tiltCounter;
}
