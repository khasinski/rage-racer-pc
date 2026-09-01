#include "game/prim.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/replay_internal.h"
#include "game/state.h"
#include "game/player_car_internal.h"
#include "game/track_internal.h"

#define AVG(a, b) (((a) + (b)) / 2)


void ApplyReplayFrame(s32 subframe, ReplayCarState *playerObj, ReplayCarState *rivalObj) {
    s32 index;
    ReplayGrandPrixFrame *big;
    ReplayTimeAttackFrame *small;
    if (g_GrandPrixMode != 0) {
        u16 playerModel = g_ReplayPlayerModel.model;
        u16 rivalModel = g_ReplayRivalModel.model;
        playerObj->modelIndex = playerModel;
        rivalObj->modelIndex = rivalModel;
        if ((subframe & 1) == 0) {
            index = subframe >> 1;
            big = &g_ReplayFramesGp[index];
            playerObj->x = big->x0;
            playerObj->y = big->y0;
            playerObj->z = big->z0;
            playerObj->modelY = big->modelY0;
            playerObj->bodyPitch = big->bodyPitch0;
            playerObj->bodyYaw = big->bodyYaw0;
            playerObj->bodyRoll = big->bodyRoll0;
            playerObj->wheelRotation = big->wheelRotation0;
            playerObj->steeringAngle = big->steeringAngle0;
            rivalObj->x = big->x1;
            rivalObj->y = big->y1;
            rivalObj->z = big->z1;
            rivalObj->modelY = big->modelY1;
            rivalObj->bodyPitch = big->bodyPitch1;
            rivalObj->bodyYaw = big->bodyYaw1;
            rivalObj->bodyRoll = big->bodyRoll1;
            rivalObj->wheelRotation = big->wheelRotation1;
            rivalObj->steeringAngle = big->steeringAngle1;
        } else {
            subframe = subframe >> 1;
            subframe += 1;
            if (subframe == 0x2EE) {
                subframe = 0;
            }
            big = &g_ReplayFramesGp[subframe];
            playerObj->x = AVG(big->x0, playerObj->x);
            playerObj->y = AVG(big->y0, playerObj->y);
            playerObj->z = AVG(big->z0, playerObj->z);
            playerObj->modelY = AVG(big->modelY0, playerObj->modelY);
            playerObj->bodyPitch = AVG(big->bodyPitch0, playerObj->bodyPitch);
            playerObj->bodyYaw = AVG(big->bodyYaw0, playerObj->bodyYaw);
            playerObj->bodyRoll = AVG(big->bodyRoll0, playerObj->bodyRoll);
            playerObj->wheelRotation = AVG(big->wheelRotation0, playerObj->wheelRotation);
            playerObj->steeringAngle = AVG(big->steeringAngle0, playerObj->steeringAngle);
            rivalObj->x = AVG(big->x1, rivalObj->x);
            rivalObj->y = AVG(big->y1, rivalObj->y);
            rivalObj->z = AVG(big->z1, rivalObj->z);
            rivalObj->modelY = AVG(big->modelY1, rivalObj->modelY);
            rivalObj->bodyPitch = AVG(big->bodyPitch1, rivalObj->bodyPitch);
            rivalObj->bodyYaw = AVG(big->bodyYaw1, rivalObj->bodyYaw);
            rivalObj->bodyRoll = AVG(big->bodyRoll1, rivalObj->bodyRoll);
            rivalObj->wheelRotation = AVG(big->wheelRotation1, rivalObj->wheelRotation);
            rivalObj->steeringAngle = AVG(big->steeringAngle1, rivalObj->steeringAngle);
        }
        playerObj->tiltCounter = big->tiltCounter;
    } else {
        playerObj->modelIndex = g_ReplayPlayerModel.model;
        if ((subframe & 1) == 0) {
            index = subframe >> 1;
            small = &g_ReplayFramesTimeAttack[index];
            playerObj->x = small->x;
            playerObj->y = small->y;
            playerObj->z = small->z;
            playerObj->modelY = small->modelY;
            playerObj->bodyPitch = small->bodyPitch;
            playerObj->bodyYaw = small->bodyYaw;
            playerObj->bodyRoll = small->bodyRoll;
            playerObj->wheelRotation = small->wheelRotation;
            playerObj->steeringAngle = small->steeringAngle;
        } else {
            subframe = subframe >> 1;
            subframe += 1;
            if (subframe == 0x505) {
                subframe = 0;
            }
            small = &g_ReplayFramesTimeAttack[subframe];
            playerObj->x = AVG(small->x, playerObj->x);
            playerObj->y = AVG(small->y, playerObj->y);
            playerObj->z = AVG(small->z, playerObj->z);
            playerObj->modelY = AVG(small->modelY, playerObj->modelY);
            playerObj->bodyPitch = AVG(small->bodyPitch, playerObj->bodyPitch);
            playerObj->bodyYaw = AVG(small->bodyYaw, playerObj->bodyYaw);
            playerObj->bodyRoll = AVG(small->bodyRoll, playerObj->bodyRoll);
            playerObj->wheelRotation = AVG(small->wheelRotation, playerObj->wheelRotation);
            playerObj->steeringAngle = AVG(small->steeringAngle, playerObj->steeringAngle);
        }
        playerObj->tiltCounter = small->tiltCounter;
    }
}

void ApplyReplayFrameAndTilt(s32 subframe, ReplayCarState *playerObj,
                             ReplayCarState *rivalObj) {
    s32 frameIndex;

    ApplyReplayFrame(subframe, playerObj, rivalObj);

    if (g_GrandPrixMode != 0) {
        if ((subframe & 1) == 0) {
            frameIndex = subframe >> 1;
        } else {
            frameIndex = (subframe >> 1) + 1;
            if (frameIndex == 0x2EE) {
                frameIndex = 0;
            }
        }
        playerObj->trackPointIndex =
            g_ReplayFramesGp[frameIndex].trackPointIndex0;
        rivalObj->trackPointIndex =
            g_ReplayFramesGp[frameIndex].trackPointIndex1;
    } else {
        if ((subframe & 1) == 0) {
            frameIndex = subframe >> 1;
        } else {
            frameIndex = (subframe >> 1) + 1;
            if (frameIndex == 0x505) {
                frameIndex = 0;
            }
        }
        playerObj->trackPointIndex =
            g_ReplayFramesTimeAttack[frameIndex].trackPointIndex;
    }
}

void RecordReplayFrame(void) {
    const GameRenderSourcePoint *player =
        (const GameRenderSourcePoint *)(const void *)&g_PlayerCar;

    if (g_GrandPrixMode != 0) {
        const GameRenderSourcePoint *rival =
            (const GameRenderSourcePoint *)(const void *)g_Cars;

        StoreReplayCarFrame(g_ReplayWriteCursor, player, rival);
    } else {
        StoreReplayTimeAttackFrame(g_ReplayWriteCursor, player);
    }

    g_ReplayWriteCursor++;
    if (g_ReplayWriteCursor == g_ReplayFrameCount) {
        g_ReplayWriteCursor = 0;
        g_ReplayBufferWrapped = 1;
    }
}

void BeginReplay(void) {
    g_FadeLevel = 0xFF;
    g_SceneTimer = 0;
    g_FadeStep = -4;

    if (g_ReplayBufferWrapped != 0) {
        g_ReplayReadCursor = (g_ReplayWriteCursor & -2) + 2;
    } else {
        g_ReplayReadCursor = 0;
        g_ReplayFrameCount = g_ReplayWriteCursor - 2;
    }

    if (g_ReplayReadCursor >= g_ReplayFrameCount) {
        g_ReplayReadCursor = 0;
    }

    if (g_GrandPrixMode != 0) {
        if (g_GrandPrixClass != 5) {
            SeekEnvironmentScript(g_EnvScriptClock - 1800);
        }
    } else {
        if (g_GrandPrixClass != 5) {
            SeekEnvironmentScript(g_EnvScriptClock - 3000);
        }
    }

    SeedReplayCars();
}

void DrawReplayBadge(void) {
    u8 *base;
    u8 *next;

    if ((g_SceneTimer & 0x10) && (g_SeriesCleared == 0)) {
        base = (u8 *)GamePrimaryOrderingTable(0);
        next = GameQueueSprite(base, RENDER_PRIM_CURSOR_AS(u8),
                               0x10, 0x10, 0x48, 0x10,
                               0, 0x68, 0x780D);
        RENDER_PRIM_CURSOR_AS(u8) = QueueDrawModePrim(base, next, 9);
    }
}
