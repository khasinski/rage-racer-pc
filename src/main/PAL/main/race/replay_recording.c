#include "game/car.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/replay_internal.h"
#include "game/work_buffer.h"

_Static_assert(REPLAY_RIVAL_COUNT == RACE_CAR_SLOT_COUNT,
               "replay must retain every rival slot");

static void StoreGrandPrixReplaySample(s32 subframe,
                                       const GameCarRuntime *player,
                                       const GameCarRuntime *rivals);
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
        StoreGrandPrixReplaySample(g_Replay.write, player, g_Cars);
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
                                       const GameCarRuntime *rivals) {
    ReplayGrandPrixFrame *dst;
    s32 i;

    g_Replay.playerModel = player->modelIndex;
    g_Replay.rivalModel = rivals[0].modelIndex;
    if ((subframe & 1) != 0) {
        return;
    }

    dst = &g_ReplayFrameBuffer.grandPrixReplay[subframe >> 1];
    dst->player = (ReplayCarFrame){
        player->x, player->y, player->z, player->modelY,
        player->bodyPitch, player->bodyYaw, player->bodyRoll,
        player->wheelRotation, player->trackPointIndex,
        player->steeringAngle, player->modelIndex,
        player->activeFlag, player->aiEnabled,
    };
    for (i = 0; i < REPLAY_RIVAL_COUNT; i++) {
        const GameCarRuntime *car = &rivals[i];
        dst->rivals[i] = (ReplayCarFrame){
            car->x, car->y, car->z, car->modelY,
            car->bodyPitch, car->bodyYaw, car->bodyRoll,
            car->wheelRotation, car->trackPointIndex,
            car->steeringAngle, car->modelIndex,
            car->activeFlag, car->aiEnabled,
        };
    }
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
