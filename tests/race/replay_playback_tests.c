#include <assert.h>
#include <string.h>

#include "game/car.h"
#include "game/race.h"
#include "game/replay_internal.h"

enum {
    GRAND_PRIX_FRAME_COUNT = 0x2EE,
    TIME_ATTACK_FRAME_COUNT = 0x505,
};

static ReplayGrandPrixFrame s_GrandPrixFrames[GRAND_PRIX_FRAME_COUNT];
static ReplayTimeAttackFrame s_TimeAttackFrames[TIME_ATTACK_FRAME_COUNT];

s16 g_GrandPrixMode;
ReplayGrandPrixFrame *g_ReplayFramesGp = s_GrandPrixFrames;
ReplayTimeAttackFrame *g_ReplayFramesTimeAttack = s_TimeAttackFrames;
ReplayModelValue g_ReplayPlayerModel;
ReplayModelValue g_ReplayRivalModel;

static void TestGrandPrixFrames(void) {
    GameCarRuntime player = {0};
    GameCarRuntime rival = {0};
    ReplayGrandPrixFrame *first = &s_GrandPrixFrames[0];
    ReplayGrandPrixFrame *second = &s_GrandPrixFrames[1];

    g_GrandPrixMode = 1;
    g_ReplayPlayerModel.model = 12;
    g_ReplayRivalModel.model = 34;
    first->x0 = 100;
    first->y0 = -20;
    first->bodyYaw0 = 30;
    first->steeringAngle0 = -8;
    first->x1 = 400;
    first->bodyRoll1 = -40;
    first->trackPointIndex0 = 51;
    first->trackPointIndex1 = 52;
    first->tiltCounter = 53;

    ApplyReplayFrameAndTrackPoint(0, &player, &rival);
    assert(player.modelIndex == 12);
    assert(rival.modelIndex == 34);
    assert(player.x == 100);
    assert(player.y == -20);
    assert(player.bodyYaw == 30);
    assert(player.steeringAngle == -8);
    assert(rival.x == 400);
    assert(rival.bodyRoll == -40);
    assert(player.tiltCounter == 53);
    assert(player.trackPointIndex == 51);
    assert(rival.trackPointIndex == 52);

    second->x0 = 200;
    second->y0 = -10;
    second->bodyYaw0 = -9;
    second->steeringAngle0 = 10;
    second->x1 = 600;
    second->bodyRoll1 = 20;
    second->trackPointIndex0 = 61;
    second->trackPointIndex1 = 62;
    second->tiltCounter = 63;

    ApplyReplayFrameAndTrackPoint(1, &player, &rival);
    assert(player.x == 150);
    assert(player.y == -15);
    assert(player.bodyYaw == 10);
    assert(player.steeringAngle == 1);
    assert(rival.x == 500);
    assert(rival.bodyRoll == -10);
    assert(player.tiltCounter == 63);
    assert(player.trackPointIndex == 61);
    assert(rival.trackPointIndex == 62);

    player.x = 300;
    rival.x = 500;
    ApplyReplayFrameAndTrackPoint(GRAND_PRIX_FRAME_COUNT * 2 - 1,
                                  &player, &rival);
    assert(player.x == 200);
    assert(rival.x == 450);
    assert(player.trackPointIndex == 51);
    assert(rival.trackPointIndex == 52);
}

static void TestTimeAttackFrames(void) {
    GameCarRuntime player = {0};
    GameCarRuntime rival;
    ReplayTimeAttackFrame *first = &s_TimeAttackFrames[0];
    ReplayTimeAttackFrame *second = &s_TimeAttackFrames[1];

    memset(&rival, 0x5A, sizeof(rival));
    g_GrandPrixMode = 0;
    g_ReplayPlayerModel.model = 7;
    first->x = 80;
    first->y = -30;
    first->wheelRotation = 12;
    first->steeringAngle = -6;
    first->trackPointIndex = 71;
    first->tiltCounter = 72;

    ApplyReplayFrameAndTrackPoint(0, &player, &rival);
    assert(player.modelIndex == 7);
    assert(player.x == 80);
    assert(player.y == -30);
    assert(player.wheelRotation == 12);
    assert(player.steeringAngle == -6);
    assert(player.trackPointIndex == 71);
    assert(player.tiltCounter == 72);
    assert(((unsigned char *)&rival)[0] == 0x5A);

    second->x = 120;
    second->y = 10;
    second->wheelRotation = -8;
    second->steeringAngle = 8;
    second->trackPointIndex = 81;
    second->tiltCounter = 82;

    ApplyReplayFrameAndTrackPoint(1, &player, &rival);
    assert(player.x == 100);
    assert(player.y == -10);
    assert(player.wheelRotation == 2);
    assert(player.steeringAngle == 1);
    assert(player.trackPointIndex == 81);
    assert(player.tiltCounter == 82);

    player.x = 200;
    ApplyReplayFrameAndTrackPoint(TIME_ATTACK_FRAME_COUNT * 2 - 1,
                                  &player, &rival);
    assert(player.x == 140);
    assert(player.trackPointIndex == 71);
}

int main(void) {
    TestGrandPrixFrames();
    TestTimeAttackFrames();
    return 0;
}
