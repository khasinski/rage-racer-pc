#include "game/car.h"
#include "game/race.h"
#include "game/replay_internal.h"

typedef struct ReplayCarPose {
    s32 x;
    s32 y;
    s32 z;
    s32 modelY;
    s32 bodyPitch;
    s32 bodyYaw;
    s32 bodyRoll;
    s32 wheelRotation;
    s32 steeringAngle;
} ReplayCarPose;

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

static ReplayCarPose GrandPrixPlayerPose(const ReplayGrandPrixFrame *frame) {
    const ReplayCarPose pose = {
        .x = frame->x0,
        .y = frame->y0,
        .z = frame->z0,
        .modelY = frame->modelY0,
        .bodyPitch = frame->bodyPitch0,
        .bodyYaw = frame->bodyYaw0,
        .bodyRoll = frame->bodyRoll0,
        .wheelRotation = frame->wheelRotation0,
        .steeringAngle = frame->steeringAngle0,
    };
    return pose;
}

static ReplayCarPose GrandPrixRivalPose(const ReplayGrandPrixFrame *frame) {
    const ReplayCarPose pose = {
        .x = frame->x1,
        .y = frame->y1,
        .z = frame->z1,
        .modelY = frame->modelY1,
        .bodyPitch = frame->bodyPitch1,
        .bodyYaw = frame->bodyYaw1,
        .bodyRoll = frame->bodyRoll1,
        .wheelRotation = frame->wheelRotation1,
        .steeringAngle = frame->steeringAngle1,
    };
    return pose;
}

static ReplayCarPose TimeAttackPlayerPose(const ReplayTimeAttackFrame *frame) {
    const ReplayCarPose pose = {
        .x = frame->x,
        .y = frame->y,
        .z = frame->z,
        .modelY = frame->modelY,
        .bodyPitch = frame->bodyPitch,
        .bodyYaw = frame->bodyYaw,
        .bodyRoll = frame->bodyRoll,
        .wheelRotation = frame->wheelRotation,
        .steeringAngle = frame->steeringAngle,
    };
    return pose;
}

static void ApplyReplayPose(GameCarRuntime *car, const ReplayCarPose *pose,
                            s32 interpolate) {
    if (interpolate != 0) {
        car->x = AverageReplayValue(pose->x, car->x);
        car->y = AverageReplayValue(pose->y, car->y);
        car->z = AverageReplayValue(pose->z, car->z);
        car->modelY = AverageReplayValue(pose->modelY, car->modelY);
        car->bodyPitch = AverageReplayValue(pose->bodyPitch, car->bodyPitch);
        car->bodyYaw = AverageReplayValue(pose->bodyYaw, car->bodyYaw);
        car->bodyRoll = AverageReplayValue(pose->bodyRoll, car->bodyRoll);
        car->wheelRotation =
            AverageReplayValue(pose->wheelRotation, car->wheelRotation);
        car->steeringAngle =
            AverageReplayValue(pose->steeringAngle, car->steeringAngle);
        return;
    }

    car->x = pose->x;
    car->y = pose->y;
    car->z = pose->z;
    car->modelY = pose->modelY;
    car->bodyPitch = pose->bodyPitch;
    car->bodyYaw = pose->bodyYaw;
    car->bodyRoll = pose->bodyRoll;
    car->wheelRotation = pose->wheelRotation;
    car->steeringAngle = pose->steeringAngle;
}

static void ApplyReplayFrameState(s32 subframe, GameCarRuntime *player,
                                  GameCarRuntime *rival,
                                  s32 restoreTrackPoint) {
    const s32 interpolate = subframe & 1;

    player->modelIndex = g_ReplayPlayerModel.model;
    if (g_GrandPrixMode != 0) {
        const s32 index = ReplaySampleIndex(
            subframe, GRAND_PRIX_REPLAY_SAMPLE_COUNT);
        const ReplayGrandPrixFrame *frame = &g_ReplayFramesGp[index];
        const ReplayCarPose playerPose = GrandPrixPlayerPose(frame);
        const ReplayCarPose rivalPose = GrandPrixRivalPose(frame);

        rival->modelIndex = g_ReplayRivalModel.model;
        ApplyReplayPose(player, &playerPose, interpolate);
        ApplyReplayPose(rival, &rivalPose, interpolate);
        player->tiltCounter = frame->tiltCounter;
        if (restoreTrackPoint != 0) {
            player->trackPointIndex = frame->trackPointIndex0;
            rival->trackPointIndex = frame->trackPointIndex1;
        }
    } else {
        const s32 index = ReplaySampleIndex(
            subframe, TIME_ATTACK_REPLAY_SAMPLE_COUNT);
        const ReplayTimeAttackFrame *frame = &g_ReplayFramesTimeAttack[index];
        const ReplayCarPose playerPose = TimeAttackPlayerPose(frame);

        ApplyReplayPose(player, &playerPose, interpolate);
        player->tiltCounter = frame->tiltCounter;
        if (restoreTrackPoint != 0) {
            player->trackPointIndex = frame->trackPointIndex;
        }
    }
}

void ApplyReplayFrame(s32 subframe, GameCarRuntime *player,
                      GameCarRuntime *rival) {
    ApplyReplayFrameState(subframe, player, rival, 0);
}

void ApplyReplayFrameAndTrackPoint(s32 subframe, GameCarRuntime *player,
                                   GameCarRuntime *rival) {
    ApplyReplayFrameState(subframe, player, rival, 1);
}
