#include <assert.h>
#include <limits.h>
#include <string.h>

#include "game/car.h"
#include "game/race.h"
#include "game/replay_internal.h"
#include "game/work_buffer.h"

GameWorkBuffer g_ReplayFrameBuffer;
Replay g_Replay;
s16 g_GrandPrixMode;

static void TestGrandPrixFrames(void) {
    GameCarRuntime player = {0};
    GameCarRuntime rivals[REPLAY_RIVAL_COUNT] = {{0}};
    ReplayGrandPrixFrame *first = &g_ReplayFrameBuffer.grandPrixReplay[0];
    ReplayGrandPrixFrame *second = &g_ReplayFrameBuffer.grandPrixReplay[1];

    g_GrandPrixMode = 1;
    g_Replay.playerModel = 12;
    g_Replay.rivalModel = 34;
    first->player.modelIndex = 12;
    first->rivals[0].modelIndex = 34;
    first->player.brakeInput = 256;
    first->rivals[0].brakeInput = 128;
    first->player.x = 100;
    first->player.y = -20;
    first->player.z = 300;
    first->player.modelY = 40;
    first->player.bodyPitch = -5;
    first->player.bodyYaw = 30;
    first->player.bodyRoll = 6;
    first->player.wheelRotation = 70;
    first->player.steeringAngle = -8;
    first->rivals[0].x = 400;
    first->rivals[0].y = 410;
    first->rivals[0].z = 420;
    first->rivals[0].modelY = 43;
    first->rivals[0].bodyPitch = -44;
    first->rivals[0].bodyYaw = 45;
    first->rivals[0].bodyRoll = -40;
    first->rivals[0].wheelRotation = 46;
    first->rivals[0].steeringAngle = -47;
    first->player.trackPointIndex = 51;
    first->rivals[0].trackPointIndex = 52;
    first->rivals[3].x = 700;
    first->rivals[3].trackPointIndex = 73;
    first->rivals[3].modelIndex = 45;
    first->rivals[3].activeFlag = 1;
    first->rivals[3].aiEnabled = 1;
    first->rivals[7].activeFlag = -1;
    first->rivals[7].aiEnabled = 0;
    first->tiltCounter = 53;

    player.trackPointIndex = 901;
    rivals[0].trackPointIndex = 902;
    ApplyReplayFrame(0, &player, rivals);
    assert(player.trackPointIndex == 901);
    assert(rivals[0].trackPointIndex == 902);

    ApplyReplayFrameAndTrackPoint(0, &player, rivals);
    assert(player.modelIndex == 12);
    assert(rivals[0].modelIndex == 34);
    assert(player.brakeInput == 256);
    assert(rivals[0].brakeInput == 128);
    assert(player.x == 100);
    assert(player.y == -20);
    assert(player.z == 300);
    assert(player.modelY == 40);
    assert(player.bodyPitch == -5);
    assert(player.bodyYaw == 30);
    assert(player.bodyRoll == 6);
    assert(player.wheelRotation == 70);
    assert(player.steeringAngle == -8);
    assert(rivals[0].x == 400);
    assert(rivals[0].y == 410);
    assert(rivals[0].z == 420);
    assert(rivals[0].modelY == 43);
    assert(rivals[0].bodyPitch == -44);
    assert(rivals[0].bodyYaw == 45);
    assert(rivals[0].bodyRoll == -40);
    assert(rivals[0].wheelRotation == 46);
    assert(rivals[0].steeringAngle == -47);
    assert(player.tiltCounter == 53);
    assert(player.trackPointIndex == 51);
    assert(rivals[0].trackPointIndex == 52);
    assert(rivals[3].x == 700);
    assert(rivals[3].trackPointIndex == 73);
    assert(rivals[3].modelIndex == 45);
    assert(rivals[3].activeFlag == 1 && rivals[3].aiEnabled == 1);
    assert(rivals[7].activeFlag == -1 && rivals[7].aiEnabled == 0);

    second->player.brakeInput = 0;
    second->rivals[0].brakeInput = 0;
    second->player.x = 200;
    second->player.modelIndex = 12;
    second->player.y = -10;
    second->player.z = 500;
    second->player.modelY = 60;
    second->player.bodyPitch = 5;
    second->player.bodyYaw = -9;
    second->player.bodyRoll = 10;
    second->player.wheelRotation = 90;
    second->player.steeringAngle = 10;
    second->rivals[0].x = 600;
    second->rivals[0].modelIndex = 34;
    second->rivals[0].y = 610;
    second->rivals[0].z = 620;
    second->rivals[0].modelY = 63;
    second->rivals[0].bodyPitch = 64;
    second->rivals[0].bodyYaw = -65;
    second->rivals[0].bodyRoll = 20;
    second->rivals[0].wheelRotation = 66;
    second->rivals[0].steeringAngle = 67;
    second->player.trackPointIndex = 61;
    second->rivals[0].trackPointIndex = 62;
    second->rivals[3] = first->rivals[3];
    second->rivals[3].x = 900;
    second->rivals[3].trackPointIndex = 83;
    second->tiltCounter = 63;

    ApplyReplayFrameAndTrackPoint(1, &player, rivals);
    assert(player.x == 150);
    assert(player.brakeInput == 256);
    assert(rivals[0].brakeInput == 128);
    assert(player.y == -15);
    assert(player.z == 400);
    assert(player.modelY == 50);
    assert(player.bodyPitch == 0);
    assert(player.bodyYaw == 10);
    assert(player.bodyRoll == 8);
    assert(player.wheelRotation == 80);
    assert(player.steeringAngle == 1);
    assert(rivals[0].x == 500);
    assert(rivals[0].y == 510);
    assert(rivals[0].z == 520);
    assert(rivals[0].modelY == 53);
    assert(rivals[0].bodyPitch == 10);
    assert(rivals[0].bodyYaw == -10);
    assert(rivals[0].bodyRoll == -10);
    assert(rivals[0].wheelRotation == 56);
    assert(rivals[0].steeringAngle == 10);
    assert(player.tiltCounter == 63);
    assert(player.trackPointIndex == 61);
    assert(rivals[0].trackPointIndex == 62);
    assert(rivals[3].x == 800);
    assert(rivals[3].trackPointIndex == 83);

    ApplyReplayFrame(2, &player, rivals);
    assert(player.brakeInput == 0 && rivals[0].brakeInput == 0);
    player.x = 300;
    rivals[0].x = 500;
    ApplyReplayFrameAndTrackPoint(GRAND_PRIX_REPLAY_SUBFRAME_COUNT - 1,
                                  &player, rivals);
    assert(player.x == 200);
    assert(rivals[0].x == 450);
    assert(player.trackPointIndex == 51);
    assert(rivals[0].trackPointIndex == 52);
}

static void TestTimeAttackFrames(void) {
    GameCarRuntime player = {0};
    GameCarRuntime rivals[REPLAY_RIVAL_COUNT];
    ReplayTimeAttackFrame *first =
        &g_ReplayFrameBuffer.timeAttackReplay[0];
    ReplayTimeAttackFrame *second =
        &g_ReplayFrameBuffer.timeAttackReplay[1];

    memset(rivals, 0x5A, sizeof(rivals));
    g_GrandPrixMode = 0;
    g_Replay.playerModel = 7;
    first->brakeInput = 256;
    second->brakeInput = 0;
    first->x = 80;
    first->y = -30;
    first->z = 160;
    first->modelY = 20;
    first->bodyPitch = -4;
    first->bodyYaw = 6;
    first->bodyRoll = -10;
    first->wheelRotation = 12;
    first->steeringAngle = -6;
    first->trackPointIndex = 71;
    first->tiltCounter = 72;

    player.trackPointIndex = 903;
    ApplyReplayFrame(0, &player, rivals);
    assert(player.trackPointIndex == 903);

    ApplyReplayFrameAndTrackPoint(0, &player, rivals);
    assert(player.modelIndex == 7);
    assert(player.brakeInput == 256);
    assert(player.x == 80);
    assert(player.y == -30);
    assert(player.z == 160);
    assert(player.modelY == 20);
    assert(player.bodyPitch == -4);
    assert(player.bodyYaw == 6);
    assert(player.bodyRoll == -10);
    assert(player.wheelRotation == 12);
    assert(player.steeringAngle == -6);
    assert(player.trackPointIndex == 71);
    assert(player.tiltCounter == 72);
    assert(((unsigned char *)rivals)[0] == 0x5A);

    second->x = 120;
    second->y = 10;
    second->z = 240;
    second->modelY = 40;
    second->bodyPitch = 4;
    second->bodyYaw = -8;
    second->bodyRoll = 20;
    second->wheelRotation = -8;
    second->steeringAngle = 8;
    second->trackPointIndex = 81;
    second->tiltCounter = 82;

    ApplyReplayFrameAndTrackPoint(1, &player, rivals);
    assert(player.x == 100);
    assert(player.brakeInput == 256);
    assert(player.y == -10);
    assert(player.z == 200);
    assert(player.modelY == 30);
    assert(player.bodyPitch == 0);
    assert(player.bodyYaw == -1);
    assert(player.bodyRoll == 5);
    assert(player.wheelRotation == 2);
    assert(player.steeringAngle == 1);
    assert(player.trackPointIndex == 81);
    assert(player.tiltCounter == 82);

    ApplyReplayFrame(2, &player, rivals);
    assert(player.brakeInput == 0);
    player.x = 200;
    ApplyReplayFrameAndTrackPoint(TIME_ATTACK_REPLAY_SUBFRAME_COUNT - 1,
                                  &player, rivals);
    assert(player.x == 140);
    assert(player.trackPointIndex == 71);
}

static void TestInterpolationUsesDefinedMachineWrapping(void) {
    GameCarRuntime player = {0};
    GameCarRuntime rivals[REPLAY_RIVAL_COUNT] = {{0}};
    ReplayTimeAttackFrame *frame =
        &g_ReplayFrameBuffer.timeAttackReplay[1];

    memset(frame, 0, sizeof(*frame));
    g_GrandPrixMode = 0;
    frame->x = UINT16_MAX;
    player.x = INT_MAX;

    ApplyReplayFrame(1, &player, rivals);

    assert(player.x == -1073709057);

    frame->bodyPitch = INT16_MIN;
    player.bodyPitch = INT_MIN;
    ApplyReplayFrame(1, &player, rivals);
    assert(player.bodyPitch == 1073725440);
}

static void TestInvalidFramesAreIgnored(void) {
    GameCarRuntime player;
    GameCarRuntime rivals[REPLAY_RIVAL_COUNT];
    GameCarRuntime originalPlayer;
    GameCarRuntime originalRivals[REPLAY_RIVAL_COUNT];

    memset(&player, 0x3C, sizeof(player));
    memset(rivals, 0x5A, sizeof(rivals));
    originalPlayer = player;
    memcpy(originalRivals, rivals, sizeof(rivals));

    g_GrandPrixMode = 1;
    ApplyReplayFrame(-1, &player, rivals);
    assert(memcmp(&player, &originalPlayer, sizeof(player)) == 0);
    assert(memcmp(rivals, originalRivals, sizeof(rivals)) == 0);

    ApplyReplayFrame(GRAND_PRIX_REPLAY_SUBFRAME_COUNT, &player, rivals);
    assert(memcmp(&player, &originalPlayer, sizeof(player)) == 0);
    ApplyReplayFrame(0, &player, NULL);
    assert(memcmp(&player, &originalPlayer, sizeof(player)) == 0);

    g_GrandPrixMode = 0;
    ApplyReplayFrame(TIME_ATTACK_REPLAY_SUBFRAME_COUNT, &player, rivals);
    assert(memcmp(&player, &originalPlayer, sizeof(player)) == 0);
    ApplyReplayFrame(0, NULL, rivals);
    assert(memcmp(rivals, originalRivals, sizeof(rivals)) == 0);
}

int main(void) {
    TestGrandPrixFrames();
    TestTimeAttackFrames();
    TestInterpolationUsesDefinedMachineWrapping();
    TestInvalidFramesAreIgnored();
    return 0;
}
