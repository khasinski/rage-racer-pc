#include "game/car.h"
#include "game/race.h"
#include "game/replay_internal.h"

static s32 ReplaySampleIndex(s32 subframe, s32 frameCount) {
    s32 index = subframe >> 1;

    if ((subframe & 1) != 0) {
        index++;
        if (index == frameCount) {
            index = 0;
        }
    }
    return index;
}

static s32 AverageReplayValue(s32 recorded, s32 current) {
    return (recorded + current) / 2;
}

static void ApplyGrandPrixPlayer(const ReplayGrandPrixFrame *frame,
                                 GameCarRuntime *car, s32 interpolate) {
    if (interpolate != 0) {
        car->x = AverageReplayValue(frame->x0, car->x);
        car->y = AverageReplayValue(frame->y0, car->y);
        car->z = AverageReplayValue(frame->z0, car->z);
        car->modelY = AverageReplayValue(frame->modelY0, car->modelY);
        car->bodyPitch = AverageReplayValue(frame->bodyPitch0, car->bodyPitch);
        car->bodyYaw = AverageReplayValue(frame->bodyYaw0, car->bodyYaw);
        car->bodyRoll = AverageReplayValue(frame->bodyRoll0, car->bodyRoll);
        car->wheelRotation =
            AverageReplayValue(frame->wheelRotation0, car->wheelRotation);
        car->steeringAngle =
            AverageReplayValue(frame->steeringAngle0, car->steeringAngle);
        return;
    }

    car->x = frame->x0;
    car->y = frame->y0;
    car->z = frame->z0;
    car->modelY = frame->modelY0;
    car->bodyPitch = frame->bodyPitch0;
    car->bodyYaw = frame->bodyYaw0;
    car->bodyRoll = frame->bodyRoll0;
    car->wheelRotation = frame->wheelRotation0;
    car->steeringAngle = frame->steeringAngle0;
}

static void ApplyGrandPrixRival(const ReplayGrandPrixFrame *frame,
                                GameCarRuntime *car, s32 interpolate) {
    if (interpolate != 0) {
        car->x = AverageReplayValue(frame->x1, car->x);
        car->y = AverageReplayValue(frame->y1, car->y);
        car->z = AverageReplayValue(frame->z1, car->z);
        car->modelY = AverageReplayValue(frame->modelY1, car->modelY);
        car->bodyPitch = AverageReplayValue(frame->bodyPitch1, car->bodyPitch);
        car->bodyYaw = AverageReplayValue(frame->bodyYaw1, car->bodyYaw);
        car->bodyRoll = AverageReplayValue(frame->bodyRoll1, car->bodyRoll);
        car->wheelRotation =
            AverageReplayValue(frame->wheelRotation1, car->wheelRotation);
        car->steeringAngle =
            AverageReplayValue(frame->steeringAngle1, car->steeringAngle);
        return;
    }

    car->x = frame->x1;
    car->y = frame->y1;
    car->z = frame->z1;
    car->modelY = frame->modelY1;
    car->bodyPitch = frame->bodyPitch1;
    car->bodyYaw = frame->bodyYaw1;
    car->bodyRoll = frame->bodyRoll1;
    car->wheelRotation = frame->wheelRotation1;
    car->steeringAngle = frame->steeringAngle1;
}

static void ApplyTimeAttackPlayer(const ReplayTimeAttackFrame *frame,
                                  GameCarRuntime *car, s32 interpolate) {
    if (interpolate != 0) {
        car->x = AverageReplayValue(frame->x, car->x);
        car->y = AverageReplayValue(frame->y, car->y);
        car->z = AverageReplayValue(frame->z, car->z);
        car->modelY = AverageReplayValue(frame->modelY, car->modelY);
        car->bodyPitch = AverageReplayValue(frame->bodyPitch, car->bodyPitch);
        car->bodyYaw = AverageReplayValue(frame->bodyYaw, car->bodyYaw);
        car->bodyRoll = AverageReplayValue(frame->bodyRoll, car->bodyRoll);
        car->wheelRotation =
            AverageReplayValue(frame->wheelRotation, car->wheelRotation);
        car->steeringAngle =
            AverageReplayValue(frame->steeringAngle, car->steeringAngle);
        return;
    }

    car->x = frame->x;
    car->y = frame->y;
    car->z = frame->z;
    car->modelY = frame->modelY;
    car->bodyPitch = frame->bodyPitch;
    car->bodyYaw = frame->bodyYaw;
    car->bodyRoll = frame->bodyRoll;
    car->wheelRotation = frame->wheelRotation;
    car->steeringAngle = frame->steeringAngle;
}

void ApplyReplayFrame(s32 subframe, GameCarRuntime *player,
                      GameCarRuntime *rival) {
    const s32 interpolate = subframe & 1;

    player->modelIndex = g_ReplayPlayerModel.model;
    if (g_GrandPrixMode != 0) {
        const s32 index = ReplaySampleIndex(
            subframe, GRAND_PRIX_REPLAY_SAMPLE_COUNT);
        const ReplayGrandPrixFrame *frame = &g_ReplayFramesGp[index];

        rival->modelIndex = g_ReplayRivalModel.model;
        ApplyGrandPrixPlayer(frame, player, interpolate);
        ApplyGrandPrixRival(frame, rival, interpolate);
        player->tiltCounter = frame->tiltCounter;
    } else {
        const s32 index = ReplaySampleIndex(
            subframe, TIME_ATTACK_REPLAY_SAMPLE_COUNT);
        const ReplayTimeAttackFrame *frame = &g_ReplayFramesTimeAttack[index];

        ApplyTimeAttackPlayer(frame, player, interpolate);
        player->tiltCounter = frame->tiltCounter;
    }
}

void ApplyReplayFrameAndTrackPoint(s32 subframe, GameCarRuntime *player,
                                   GameCarRuntime *rival) {
    ApplyReplayFrame(subframe, player, rival);

    if (g_GrandPrixMode != 0) {
        const s32 index = ReplaySampleIndex(
            subframe, GRAND_PRIX_REPLAY_SAMPLE_COUNT);

        player->trackPointIndex = g_ReplayFramesGp[index].trackPointIndex0;
        rival->trackPointIndex = g_ReplayFramesGp[index].trackPointIndex1;
    } else {
        const s32 index = ReplaySampleIndex(
            subframe, TIME_ATTACK_REPLAY_SAMPLE_COUNT);

        player->trackPointIndex =
            g_ReplayFramesTimeAttack[index].trackPointIndex;
    }
}
