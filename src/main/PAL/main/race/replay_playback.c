#include "game/car.h"
#include "game/integer.h"
#include "game/race.h"
#include "game/replay_internal.h"
#include "game/work_buffer.h"

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
    const s32 wrappedSum = WrapSigned32((int64_t)recorded + current);

    return wrappedSum / 2;
}

static ReplayCarPose GrandPrixCarPose(const ReplayCarFrame *frame) {
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
                                  GameCarRuntime *rivals,
                                  s32 restoreTrackPoint) {
    const s32 interpolate = subframe & 1;
    const s32 frameCount = ReplayFrameCapacity(g_GrandPrixMode);

    if (player == NULL || subframe < 0 || subframe >= frameCount ||
        (g_GrandPrixMode != 0 && rivals == NULL)) {
        return;
    }

    player->modelIndex = g_Replay.playerModel;
    if (g_GrandPrixMode != 0) {
        const s32 index = ReplaySampleIndex(
            subframe, GRAND_PRIX_REPLAY_SAMPLE_COUNT);
        const ReplayGrandPrixFrame *frame =
            &g_ReplayFrameBuffer.grandPrixReplay[index];
        const ReplayCarPose playerPose = GrandPrixCarPose(&frame->player);
        s32 i;

        ApplyReplayPose(player, &playerPose, interpolate);
        /* Pedal edges are discrete; hold the current sample on the
         * interpolated pose instead of switching half a sample early. */
        player->brakeInput = g_ReplayFrameBuffer.grandPrixReplay[subframe >> 1]
                                 .player.brakeInput;
        player->modelIndex = frame->player.modelIndex;
        player->tiltCounter = frame->tiltCounter;
        if (restoreTrackPoint != 0) {
            player->trackPointIndex = frame->player.trackPointIndex;
        }
        for (i = 0; i < REPLAY_RIVAL_COUNT; i++) {
            const ReplayCarFrame *recorded = &frame->rivals[i];
            const ReplayCarPose rivalPose = GrandPrixCarPose(recorded);
            GameCarRuntime *rival = &rivals[i];

            ApplyReplayPose(rival, &rivalPose, interpolate);
            rival->brakeInput = g_ReplayFrameBuffer.grandPrixReplay[subframe >> 1]
                                    .rivals[i].brakeInput;
            rival->modelIndex = recorded->modelIndex;
            rival->activeFlag = recorded->activeFlag;
            rival->aiEnabled = recorded->aiEnabled;
            if (restoreTrackPoint != 0) {
                rival->trackPointIndex = recorded->trackPointIndex;
            }
        }
    } else {
        const s32 index = ReplaySampleIndex(
            subframe, TIME_ATTACK_REPLAY_SAMPLE_COUNT);
        const ReplayTimeAttackFrame *frame =
            &g_ReplayFrameBuffer.timeAttackReplay[index];
        const ReplayCarPose playerPose = TimeAttackPlayerPose(frame);

        ApplyReplayPose(player, &playerPose, interpolate);
        player->tiltCounter = frame->tiltCounter;
        player->brakeInput = g_ReplayFrameBuffer.timeAttackReplay[subframe >> 1]
                                 .brakeInput;
        if (restoreTrackPoint != 0) {
            player->trackPointIndex = frame->trackPointIndex;
        }
    }
}

void ApplyReplayFrame(s32 subframe, GameCarRuntime *player,
                      GameCarRuntime *rivals) {
    ApplyReplayFrameState(subframe, player, rivals, 0);
}

void ApplyReplayFrameAndTrackPoint(s32 subframe, GameCarRuntime *player,
                                   GameCarRuntime *rivals) {
    ApplyReplayFrameState(subframe, player, rivals, 1);
}
