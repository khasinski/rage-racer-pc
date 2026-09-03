/* Regression sweep for the player's collision response and slipstream. */

#include "common.h"
#include "game/car.h"
#include "game/car_collision_internal.h"
#include "game/car_motion_internal.h"
#include "game/race.h"
#include "game/render_state.h"
#include "game/state.h"
#include "game/track.h"

#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
GameCarRuntime g_Cars[11];

static u32 s_digest = 2166136261U;
static s32 s_sound;
static s32 s_knockbackCount;
static s32 s_knockbackValues[8];

static void Fold(s32 value) {
    int byte;

    for (byte = 0; byte < 4; byte++) {
        s_digest ^= ((u32)value >> (byte * 8)) & 0xFF;
        s_digest *= 16777619U;
    }
}

void PlaySoundCue(s32 cue) {
    s_sound = cue;
}

void SetCarCollisionKnockback(GameCarRuntime *car, s32 x, s32 z) {
    int slot = s_knockbackCount * 4;
    s32 mode = CAR_KNOCKBACK_VECTOR_MODE;
    s32 carIndex = car >= g_Cars && car < g_Cars + 11
        ? (s32)(car - g_Cars)
        : -1;

    if (slot < 8) {
        s_knockbackValues[slot] = carIndex;
        s_knockbackValues[slot + 1] = x;
        s_knockbackValues[slot + 2] = z;
        s_knockbackValues[slot + 3] = mode;
    }
    s_knockbackCount++;
}

int DiagnosticsEnabled(const char *channel) {
    (void)channel;
    return 0;
}

const char *DiagnosticsValue(const char *key) {
    (void)key;
    return NULL;
}

int DiagnosticsIntValue(const char *key, int fallback) {
    (void)key;
    return fallback;
}

void Trace(const char *channel, const char *format, ...) {
    (void)channel;
    (void)format;
}

MATRIX *MulMatrix0(MATRIX *a, MATRIX *b, MATRIX *out) {
    (void)a;
    (void)b;
    return out;
}

void GameRenderWorldSetCamera(s32 x, s32 y, s32 z, s32 pitch, s32 yaw,
                              s32 roll) {
    (void)x; (void)y; (void)z; (void)pitch; (void)yaw; (void)roll;
}

static void PrepareSoundCollision(PlayerCarRuntime *player,
                                  s32 lateralDistance) {
    GameCarRuntime *opponent = &g_Cars[0];
    int index;

    memset(player, 0, sizeof(*player));
    memset(g_Cars, 0, sizeof(g_Cars));
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++)
        g_Cars[index].activeFlag = -1;

    g_GrandPrixMode = 1;
    g_RaceSeries = 0;
    g_RacePhase = 2;
    g_MirrorMode = 0;
    g_WrongWayTimer = 0;
    g_TrackLength = 0x8000;
    s_sound = -1;
    s_knockbackCount = 0;

    player->x = 0x2000;
    player->z = 0x3000;
    player->trackProgress = 0x1000;
    player->speed = 80;
    opponent->activeFlag = 0;
    opponent->x = player->x;
    opponent->z = player->z;
    opponent->trackProgress = player->trackProgress;
    opponent->trackLateralOffset = lateralDistance;
    opponent->speed = 80;
}

static int CheckCollisionSoundGates(void) {
    PlayerCarRuntime player;
    s32 normalCue;
    s32 mirrorCue;

    PrepareSoundCollision(&player, 0);
    player.motionTimer = 11;
    if (CollidePlayerWithCars(&player) <= 0 || s_sound != -1) {
        puts("FAIL collision sound was not gated by motion timer");
        return 1;
    }

    PrepareSoundCollision(&player, 0);
    g_RacePhase = 3;
    if (CollidePlayerWithCars(&player) <= 0 || s_sound != -1) {
        puts("FAIL collision sound was not gated by race phase");
        return 1;
    }

    PrepareSoundCollision(&player, 49);
    if (CollidePlayerWithCars(&player) <= 0) {
        puts("FAIL side collision fixture did not collide");
        return 1;
    }
    normalCue = s_sound;

    PrepareSoundCollision(&player, 49);
    g_MirrorMode = 1;
    if (CollidePlayerWithCars(&player) <= 0) {
        puts("FAIL mirrored side collision fixture did not collide");
        return 1;
    }
    mirrorCue = s_sound;
    if (!((normalCue == 0xB && mirrorCue == 0xC) ||
          (normalCue == 0xC && mirrorCue == 0xB))) {
        printf("FAIL mirror collision cues were %d and %d\n", normalCue,
               mirrorCue);
        return 1;
    }
    return 0;
}

static int CheckWrappedWorldCoordinates(void) {
    PlayerCarRuntime player;

    PrepareSoundCollision(&player, 0);
    player.x = 0xFFFC;
    g_Cars[0].x = 4;
    if (CollidePlayerWithCars(&player) <= 0) {
        puts("FAIL nearby cars did not collide across world-coordinate wrap");
        return 1;
    }
    return 0;
}

static int CheckWrappedTrackProgress(void) {
    PlayerCarRuntime player;

    PrepareSoundCollision(&player, 0);
    player.trackProgress = INT32_MAX - 10;
    g_Cars[0].trackProgress = INT32_MIN + 9;
    if (CollidePlayerWithCars(&player) <= 0) {
        puts("FAIL nearby cars did not collide across track-progress wrap");
        return 1;
    }
    return 0;
}

static int CheckAllCollisionFlagsReset(void) {
    PlayerCarRuntime player;

    PrepareSoundCollision(&player, 0);
    g_Cars[10].activeFlag = 0;
    g_Cars[10].collisionFlag = 1;
    if (CollidePlayerWithCars(&player) <= 0 ||
        g_Cars[0].collisionFlag != 1 || g_Cars[10].collisionFlag != 0) {
        printf("FAIL collision flags after early hit: first=%d later=%d\n",
               g_Cars[0].collisionFlag, g_Cars[10].collisionFlag);
        return 1;
    }
    return 0;
}

static int CheckMissingTrackLength(void) {
    PlayerCarRuntime player;

    PrepareSoundCollision(&player, 0);
    g_TrackLength = 0;
    g_Cars[0].collisionFlag = 1;
    if (CollidePlayerWithCars(&player) != 0 ||
        g_Cars[0].collisionFlag != 1 || s_knockbackCount != 0) {
        puts("FAIL missing track length did not leave collision state alone");
        return 1;
    }
    return 0;
}

static int CheckExtremeCoordinateDifferences(void) {
    PlayerCarRuntime player;

    PrepareSoundCollision(&player, 0);
    player.y = INT32_MIN;
    player.trackLateralOffset = INT32_MIN;
    g_Cars[0].y = INT32_MAX;
    g_Cars[0].trackLateralOffset = INT32_MAX;
    if (CollidePlayerWithCars(&player) != 0 || s_knockbackCount != 0) {
        puts("FAIL extreme coordinate differences produced a collision");
        return 1;
    }
    return 0;
}

static int CheckExtremeSpeedDifference(void) {
    PlayerCarRuntime player;
    s32 region;

    PrepareSoundCollision(&player, 0);
    player.speed = INT32_MIN;
    g_Cars[0].speed = INT32_MAX;
    g_Cars[0].x -= 40;
    g_Cars[0].z -= 80;
    region = CollidePlayerWithCars(&player);
    if (region != 1 || g_GripLossTimer != 0xF) {
        printf("FAIL extreme collision speed difference: region=%d grip=%d\n",
               region, g_GripLossTimer);
        return 1;
    }
    return 0;
}

static int CheckWrappedSlipstreamDistance(void) {
    PlayerCarRuntime player;

    PrepareSoundCollision(&player, 0);
    player.trackProgress = INT32_MAX;
    g_Cars[0].trackProgress = 0;
    g_Cars[0].y = 60;
    if (CollidePlayerWithCars(&player) != 0 || g_DragScale != -7441) {
        printf("FAIL wrapped slipstream drag is %d, expected -7441\n",
               g_DragScale);
        return 1;
    }
    return 0;
}

int main(void) {
    static const s32 progressDeltas[] = {-201, -1, 0, 100, 199, 200, 900};
    static const s32 lateralDeltas[] = {-100, -49, 0, 49, 100};
    static const s32 heights[] = {0, 25, 60};
    static const s32 yaws[] = {0, 0x400, 0x800};
    static const s32 nudges[] = {0, 40, 100, 200};
    static const s32 speeds[] = {40, 80, 500};
    static const u32 expected = 2498597253U;
    PlayerCarRuntime player;
    s32 calls = 0;
    size_t gp, active, vertical, progress, lateral, height;
    size_t playerYaw, opponentYaw, nudge, speed, backwards;

    if (CheckCollisionSoundGates() != 0 ||
        CheckWrappedWorldCoordinates() != 0 ||
        CheckWrappedTrackProgress() != 0 ||
        CheckAllCollisionFlagsReset() != 0 ||
        CheckMissingTrackLength() != 0 ||
        CheckExtremeCoordinateDifferences() != 0 ||
        CheckExtremeSpeedDifference() != 0 ||
        CheckWrappedSlipstreamDistance() != 0)
        return 1;

    g_TrackLength = 0x8000;
    for (gp = 0; gp < 2; gp++)
    for (active = 0; active < 2; active++)
    for (vertical = 0; vertical < 2; vertical++)
    for (progress = 0; progress < sizeof(progressDeltas) / sizeof(progressDeltas[0]); progress++)
    for (lateral = 0; lateral < sizeof(lateralDeltas) / sizeof(lateralDeltas[0]); lateral++)
    for (height = 0; height < sizeof(heights) / sizeof(heights[0]); height++)
    for (playerYaw = 0; playerYaw < sizeof(yaws) / sizeof(yaws[0]); playerYaw++)
    for (opponentYaw = 0; opponentYaw < sizeof(yaws) / sizeof(yaws[0]); opponentYaw++)
    for (nudge = 0; nudge < sizeof(nudges) / sizeof(nudges[0]); nudge++)
    for (speed = 0; speed < sizeof(speeds) / sizeof(speeds[0]); speed++)
    for (backwards = 0; backwards < 2; backwards++) {
        GameCarRuntime *opponent = &g_Cars[5];
        s32 result;
        int index;

        memset(&player, 0, sizeof(player));
        memset(g_Cars, 0, sizeof(GameCarRuntime) * 11);
        for (index = 0; index < 11; index++) {
            g_Cars[index].activeFlag = -1;
        }
        g_GrandPrixMode = (s16)gp;
        g_RaceSeries = 0;
        g_RacePhase = 2;
        g_MirrorMode = 0;
        g_WrongWayTimer = 12;
        g_DragScale = 1000;
        g_GripLossTimer = 0;
        s_sound = -1;
        s_knockbackCount = 0;
        memset(s_knockbackValues, 0, sizeof(s_knockbackValues));

        player.x = 0x2000;
        player.z = 0x3000;
        player.trackProgress = 0x1000;
        player.trackLateralOffset = 0;
        player.bodyYaw = yaws[playerYaw];
        player.verticalMotionState = 1;
        player.speed = speeds[speed];
        player.acceleration = 1200;
        player.drive.drivetrainTorque = 2400;
        player.drive.accelPos = 300;
        player.drive.brakePos = -200;
        player.facingBackwards = (s32)backwards;

        opponent->activeFlag = active ? -1 : 0;
        opponent->verticalMotionState = vertical ? 1 : 2;
        opponent->trackProgress = player.trackProgress + progressDeltas[progress];
        opponent->trackLateralOffset = lateralDeltas[lateral];
        opponent->y = heights[height];
        opponent->x = player.x + nudges[nudge];
        opponent->z = player.z + nudges[(nudge + 1) % 4];
        opponent->bodyYaw = yaws[opponentYaw];
        opponent->speed = 100;
        opponent->acceleration = 800;
        opponent->worldVelocityX = 700;
        opponent->worldVelocityZ = -500;
        opponent->velocityX = 7;
        opponent->velocityZ = -9;
        opponent->collisionBoostDuration = 17;

        result = CollidePlayerWithCars(&player);
        Fold(result);
        Fold(g_DragScale);
        Fold(g_GripLossTimer);
        Fold(player.acceleration);
        Fold(player.drive.drivetrainTorque);
        Fold(opponent->collisionFlag);
        Fold(opponent->speed);
        Fold(opponent->acceleration);
        Fold(opponent->boostTimer);
        Fold(s_sound);
        Fold(s_knockbackCount);
        for (index = 0; index < 8; index++) {
            Fold(s_knockbackValues[index]);
        }
        calls++;
    }

    if (s_digest != expected) {
        printf("FAIL: %d player collision states digest to %u, expected %u\n",
               calls, s_digest, expected);
        return 1;
    }
    printf("all %d player collision states preserved\n", calls);
    return 0;
}
