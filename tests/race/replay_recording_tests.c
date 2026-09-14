#include <assert.h>
#include <string.h>

#include "game/car.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/replay_internal.h"
#include "game/state.h"
#include "game/work_buffer.h"

GameWorkBuffer g_ReplayFrameBuffer;
Replay g_Replay;
s16 g_GrandPrixMode;
PlayerCarRuntime g_PlayerCar;
GameCarRuntime g_Cars[RACE_CAR_SLOT_COUNT];

static GameCarRuntime MakeCar(s32 base, s16 modelIndex) {
    GameCarRuntime car = {0};

    car.x = base + 1;
    car.y = base + 2;
    car.z = base + 3;
    car.modelY = base + 4;
    car.bodyPitch = base + 5;
    car.bodyYaw = base + 6;
    car.bodyRoll = base + 7;
    car.wheelRotation = base + 8;
    car.steeringAngle = base + 9;
    car.trackPointIndex = base + 10;
    car.tiltCounter = (s16)(base + 11);
    car.modelIndex = modelIndex;
    return car;
}

static void TestReplayBufferReset(void) {
    g_GrandPrixMode = 1;
    g_Replay.write = 99;
    g_Replay.wrapped = 1;
    ResetReplayWriteCursor();
    assert(g_Replay.write == 0);
    assert(g_Replay.count == GRAND_PRIX_REPLAY_SUBFRAME_COUNT);
    assert(g_Replay.wrapped == 0);

    g_GrandPrixMode = 0;
    ResetReplayWriteCursor();
    assert(g_Replay.count == TIME_ATTACK_REPLAY_SUBFRAME_COUNT);
}

static void TestGrandPrixRecording(void) {
    GameCarRuntime player = MakeCar(100, 3);
    GameCarRuntime rival = MakeCar(200, 4);
    ReplayGrandPrixFrame untouched;
    ReplayGrandPrixFrame *frame;
    /* Only the assertions read it, and a release build compiles those
     * away, which leaves it set but unused. */
    (void)frame;

    memset(&untouched, 0xA5, sizeof(untouched));
    g_ReplayFrameBuffer.grandPrixReplay[0] = untouched;
    *AsRivalCar(&g_PlayerCar) = player;
    g_Cars[0] = rival;
    g_Cars[4] = MakeCar(600, 9);
    g_Cars[4].activeFlag = 1;
    g_Cars[4].aiEnabled = 1;
    g_GrandPrixMode = 1;
    g_Replay.write = 1;
    g_Replay.count = GRAND_PRIX_REPLAY_SUBFRAME_COUNT;
    RecordReplayFrame();
    assert(memcmp(&g_ReplayFrameBuffer.grandPrixReplay[0], &untouched,
                  sizeof(untouched)) == 0);
    assert(g_Replay.playerModel == 3);
    assert(g_Replay.rivalModel == 4);

    assert(g_Replay.write == 2);
    RecordReplayFrame();
    frame = &g_ReplayFrameBuffer.grandPrixReplay[1];
    assert(frame->player.x == 101);
    assert(frame->player.y == 102);
    assert(frame->player.z == 103);
    assert(frame->player.modelY == 104);
    assert(frame->player.bodyPitch == 105);
    assert(frame->player.bodyYaw == 106);
    assert(frame->player.bodyRoll == 107);
    assert(frame->player.wheelRotation == 108);
    assert(frame->player.steeringAngle == 109);
    assert(frame->player.trackPointIndex == 110);
    assert(frame->tiltCounter == 111);
    assert(frame->rivals[0].x == 201);
    assert(frame->rivals[0].y == 202);
    assert(frame->rivals[0].z == 203);
    assert(frame->rivals[0].modelY == 204);
    assert(frame->rivals[0].bodyPitch == 205);
    assert(frame->rivals[0].bodyYaw == 206);
    assert(frame->rivals[0].bodyRoll == 207);
    assert(frame->rivals[0].wheelRotation == 208);
    assert(frame->rivals[0].steeringAngle == 209);
    assert(frame->rivals[0].trackPointIndex == 210);
    assert(frame->rivals[4].x == 601);
    assert(frame->rivals[4].modelIndex == 9);
    assert(frame->rivals[4].activeFlag == 1);
    assert(frame->rivals[4].aiEnabled == 1);
}

static void TestTimeAttackRecording(void) {
    GameCarRuntime player = MakeCar(300, 5);
    ReplayTimeAttackFrame untouched;
    ReplayTimeAttackFrame *frame;
    /* Only the assertions read it, and a release build compiles those
     * away, which leaves it set but unused. */
    (void)frame;

    memset(&untouched, 0x5A, sizeof(untouched));
    g_ReplayFrameBuffer.timeAttackReplay[0] = untouched;
    *AsRivalCar(&g_PlayerCar) = player;
    g_GrandPrixMode = 0;
    g_Replay.write = 1;
    g_Replay.count = TIME_ATTACK_REPLAY_SUBFRAME_COUNT;
    RecordReplayFrame();
    assert(memcmp(&g_ReplayFrameBuffer.timeAttackReplay[0], &untouched,
                  sizeof(untouched)) == 0);
    assert(g_Replay.playerModel == 5);

    assert(g_Replay.write == 2);
    RecordReplayFrame();
    frame = &g_ReplayFrameBuffer.timeAttackReplay[1];
    assert(frame->x == 301);
    assert(frame->y == 302);
    assert(frame->z == 303);
    assert(frame->modelY == 304);
    assert(frame->bodyPitch == 305);
    assert(frame->bodyYaw == 306);
    assert(frame->bodyRoll == 307);
    assert(frame->wheelRotation == 308);
    assert(frame->steeringAngle == 309);
    assert(frame->trackPointIndex == 310);
    assert(frame->tiltCounter == 311);
}

static void TestRecordingCursorWrap(void) {
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    memset(g_Cars, 0, sizeof(g_Cars));
    g_GrandPrixMode = 1;
    g_Replay.write = 1;
    g_Replay.count = 2;
    g_Replay.wrapped = 0;

    RecordReplayFrame();
    assert(g_Replay.write == 0);
    assert(g_Replay.wrapped == 1);
}

static void TestInvalidRecordingBoundsAreIgnored(void) {
    GameWorkBuffer untouched;

    memset(&g_ReplayFrameBuffer, 0x6C, sizeof(g_ReplayFrameBuffer));
    untouched = g_ReplayFrameBuffer;
    g_GrandPrixMode = 1;
    g_Replay.wrapped = 0;

    g_Replay.count = 0;
    g_Replay.write = 0;
    RecordReplayFrame();
    assert(memcmp(&g_ReplayFrameBuffer, &untouched, sizeof(untouched)) == 0);
    assert(g_Replay.write == 0 && g_Replay.wrapped == 0);

    g_Replay.count = GRAND_PRIX_REPLAY_SUBFRAME_COUNT;
    g_Replay.write = -1;
    RecordReplayFrame();
    assert(memcmp(&g_ReplayFrameBuffer, &untouched, sizeof(untouched)) == 0);
    assert(g_Replay.write == -1);

    g_Replay.count = GRAND_PRIX_REPLAY_SUBFRAME_COUNT + 1;
    g_Replay.write = 0;
    RecordReplayFrame();
    assert(memcmp(&g_ReplayFrameBuffer, &untouched, sizeof(untouched)) == 0);
}

int main(void) {
    TestReplayBufferReset();
    TestGrandPrixRecording();
    TestTimeAttackRecording();
    TestRecordingCursorWrap();
    TestInvalidRecordingBoundsAreIgnored();
    return 0;
}
