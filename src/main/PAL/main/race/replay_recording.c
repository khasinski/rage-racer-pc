#include "game/car.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/replay_internal.h"
#include "game/work_buffer.h"

static void StoreGrandPrixReplaySample(s32 subframe,
                                       const GameCarRuntime *player,
                                       const GameCarRuntime *rival);
static void StoreTimeAttackReplaySample(s32 subframe,
                                        const GameCarRuntime *player);

void RecordReplayFrame(void) {
    const GameCarRuntime *player = AsRivalCar(&g_PlayerCar);
    const s32 capacity = ReplayFrameCapacity(g_GrandPrixMode);

    if (g_Replay.count <= 0 || g_Replay.count > capacity ||
        g_Replay.write < 0 ||
        g_Replay.write >= g_Replay.count) {
        return;
    }

    if (g_GrandPrixMode != 0) {
        StoreGrandPrixReplaySample(g_Replay.write, player, &g_Cars[0]);
    } else {
        StoreTimeAttackReplaySample(g_Replay.write, player);
    }

    g_Replay.write++;
    if (g_Replay.write == g_Replay.count) {
        g_Replay.write = 0;
        g_Replay.wrapped = 1;
    }
}

void ResetReplayWriteCursor(void) {
    g_Replay.write = 0;
    g_Replay.count = ReplayFrameCapacity(g_GrandPrixMode);
    g_Replay.wrapped = 0;
}

static void StoreGrandPrixReplaySample(s32 subframe,
                                       const GameCarRuntime *player,
                                       const GameCarRuntime *rival) {
    ReplayGrandPrixFrame *dst;

    g_Replay.playerModel = player->modelIndex;
    g_Replay.rivalModel = rival->modelIndex;
    if ((subframe & 1) != 0) {
        return;
    }

    dst = &g_ReplayFrameBuffer.grandPrixReplay[subframe >> 1];
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

static void StoreTimeAttackReplaySample(s32 subframe,
                                        const GameCarRuntime *player) {
    ReplayTimeAttackFrame *dst;

    g_Replay.playerModel = player->modelIndex;
    if ((subframe & 1) != 0) {
        return;
    }

    dst = &g_ReplayFrameBuffer.timeAttackReplay[subframe >> 1];
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
