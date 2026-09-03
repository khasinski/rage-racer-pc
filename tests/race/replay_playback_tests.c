#include <assert.h>
#include <string.h>

#include "game/car.h"
#include "game/race.h"
#include "game/replay_internal.h"
#include "game/work_buffer.h"

GameWorkBuffer g_ReplayFrameBuffer;
s16 g_GrandPrixMode;
s16 g_ReplayPlayerModelIndex;
s16 g_ReplayRivalModelIndex;

static void TestGrandPrixFrames(void) {
    GameCarRuntime player = {0};
    GameCarRuntime rival = {0};
    ReplayGrandPrixFrame *first = &g_ReplayFrameBuffer.grandPrixReplay[0];
    ReplayGrandPrixFrame *second = &g_ReplayFrameBuffer.grandPrixReplay[1];

    g_GrandPrixMode = 1;
    g_ReplayPlayerModelIndex = 12;
    g_ReplayRivalModelIndex = 34;
    first->x0 = 100;
    first->y0 = -20;
    first->z0 = 300;
    first->modelY0 = 40;
    first->bodyPitch0 = -5;
    first->bodyYaw0 = 30;
    first->bodyRoll0 = 6;
    first->wheelRotation0 = 70;
    first->steeringAngle0 = -8;
    first->x1 = 400;
    first->y1 = 410;
    first->z1 = 420;
    first->modelY1 = 43;
    first->bodyPitch1 = -44;
    first->bodyYaw1 = 45;
    first->bodyRoll1 = -40;
    first->wheelRotation1 = 46;
    first->steeringAngle1 = -47;
    first->trackPointIndex0 = 51;
    first->trackPointIndex1 = 52;
    first->tiltCounter = 53;

    player.trackPointIndex = 901;
    rival.trackPointIndex = 902;
    ApplyReplayFrame(0, &player, &rival);
    assert(player.trackPointIndex == 901);
    assert(rival.trackPointIndex == 902);

    ApplyReplayFrameAndTrackPoint(0, &player, &rival);
    assert(player.modelIndex == 12);
    assert(rival.modelIndex == 34);
    assert(player.x == 100);
    assert(player.y == -20);
    assert(player.z == 300);
    assert(player.modelY == 40);
    assert(player.bodyPitch == -5);
    assert(player.bodyYaw == 30);
    assert(player.bodyRoll == 6);
    assert(player.wheelRotation == 70);
    assert(player.steeringAngle == -8);
    assert(rival.x == 400);
    assert(rival.y == 410);
    assert(rival.z == 420);
    assert(rival.modelY == 43);
    assert(rival.bodyPitch == -44);
    assert(rival.bodyYaw == 45);
    assert(rival.bodyRoll == -40);
    assert(rival.wheelRotation == 46);
    assert(rival.steeringAngle == -47);
    assert(player.tiltCounter == 53);
    assert(player.trackPointIndex == 51);
    assert(rival.trackPointIndex == 52);

    second->x0 = 200;
    second->y0 = -10;
    second->z0 = 500;
    second->modelY0 = 60;
    second->bodyPitch0 = 5;
    second->bodyYaw0 = -9;
    second->bodyRoll0 = 10;
    second->wheelRotation0 = 90;
    second->steeringAngle0 = 10;
    second->x1 = 600;
    second->y1 = 610;
    second->z1 = 620;
    second->modelY1 = 63;
    second->bodyPitch1 = 64;
    second->bodyYaw1 = -65;
    second->bodyRoll1 = 20;
    second->wheelRotation1 = 66;
    second->steeringAngle1 = 67;
    second->trackPointIndex0 = 61;
    second->trackPointIndex1 = 62;
    second->tiltCounter = 63;

    ApplyReplayFrameAndTrackPoint(1, &player, &rival);
    assert(player.x == 150);
    assert(player.y == -15);
    assert(player.z == 400);
    assert(player.modelY == 50);
    assert(player.bodyPitch == 0);
    assert(player.bodyYaw == 10);
    assert(player.bodyRoll == 8);
    assert(player.wheelRotation == 80);
    assert(player.steeringAngle == 1);
    assert(rival.x == 500);
    assert(rival.y == 510);
    assert(rival.z == 520);
    assert(rival.modelY == 53);
    assert(rival.bodyPitch == 10);
    assert(rival.bodyYaw == -10);
    assert(rival.bodyRoll == -10);
    assert(rival.wheelRotation == 56);
    assert(rival.steeringAngle == 10);
    assert(player.tiltCounter == 63);
    assert(player.trackPointIndex == 61);
    assert(rival.trackPointIndex == 62);

    player.x = 300;
    rival.x = 500;
    ApplyReplayFrameAndTrackPoint(GRAND_PRIX_REPLAY_SUBFRAME_COUNT - 1,
                                  &player, &rival);
    assert(player.x == 200);
    assert(rival.x == 450);
    assert(player.trackPointIndex == 51);
    assert(rival.trackPointIndex == 52);
}

static void TestTimeAttackFrames(void) {
    GameCarRuntime player = {0};
    GameCarRuntime rival;
    ReplayTimeAttackFrame *first =
        &g_ReplayFrameBuffer.timeAttackReplay[0];
    ReplayTimeAttackFrame *second =
        &g_ReplayFrameBuffer.timeAttackReplay[1];

    memset(&rival, 0x5A, sizeof(rival));
    g_GrandPrixMode = 0;
    g_ReplayPlayerModelIndex = 7;
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
    ApplyReplayFrame(0, &player, &rival);
    assert(player.trackPointIndex == 903);

    ApplyReplayFrameAndTrackPoint(0, &player, &rival);
    assert(player.modelIndex == 7);
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
    assert(((unsigned char *)&rival)[0] == 0x5A);

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

    ApplyReplayFrameAndTrackPoint(1, &player, &rival);
    assert(player.x == 100);
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

    player.x = 200;
    ApplyReplayFrameAndTrackPoint(TIME_ATTACK_REPLAY_SUBFRAME_COUNT - 1,
                                  &player, &rival);
    assert(player.x == 140);
    assert(player.trackPointIndex == 71);
}

int main(void) {
    TestGrandPrixFrames();
    TestTimeAttackFrames();
    return 0;
}
