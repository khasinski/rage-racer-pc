#include <assert.h>
#include <string.h>

#include "game/car.h"
#include "game/race.h"
#include "game/replay_internal.h"
#include "game/state.h"
#include "game/work_buffer.h"

GameWorkBuffer g_ReplayFrameBuffer;
ReplayGrandPrixFrame *g_ReplayFramesGp;
ReplayTimeAttackFrame *g_ReplayFramesTimeAttack;
s32 g_ReplayWriteCursor;
s32 g_ReplayFrameCount;
s32 g_ReplayBufferWrapped;
ReplayModelValue g_ReplayPlayerModel;
ReplayModelValue g_ReplayRivalModel;
s16 g_GrandPrixMode;

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
    ResetReplayFrameCounts();
    assert(g_ReplayFramesGp == g_ReplayFrameBuffer.grandPrixReplay);
    assert(g_ReplayFramesTimeAttack == g_ReplayFrameBuffer.timeAttackReplay);

    g_GrandPrixMode = 1;
    g_ReplayWriteCursor = 99;
    g_ReplayBufferWrapped = 1;
    ResetReplayWriteCursor();
    assert(g_ReplayWriteCursor == 0);
    assert(g_ReplayFrameCount == 0x5DC);
    assert(g_ReplayBufferWrapped == 0);

    g_GrandPrixMode = 0;
    ResetReplayWriteCursor();
    assert(g_ReplayFrameCount == 0xA0A);
}

static void TestGrandPrixRecording(void) {
    GameCarRuntime player = MakeCar(100, 3);
    GameCarRuntime rival = MakeCar(200, 4);
    ReplayGrandPrixFrame untouched;
    ReplayGrandPrixFrame *frame;

    memset(&untouched, 0xA5, sizeof(untouched));
    g_ReplayFrameBuffer.grandPrixReplay[0] = untouched;
    StoreReplayCarFrame(1, &player, &rival);
    assert(memcmp(&g_ReplayFrameBuffer.grandPrixReplay[0], &untouched,
                  sizeof(untouched)) == 0);
    assert(g_ReplayPlayerModel.model == 3);
    assert(g_ReplayRivalModel.model == 4);

    StoreReplayCarFrame(2, &player, &rival);
    frame = &g_ReplayFrameBuffer.grandPrixReplay[1];
    assert(frame->x0 == 101);
    assert(frame->y0 == 102);
    assert(frame->bodyYaw0 == 106);
    assert(frame->steeringAngle0 == 109);
    assert(frame->trackPointIndex0 == 110);
    assert(frame->tiltCounter == 111);
    assert(frame->x1 == 201);
    assert(frame->bodyRoll1 == 207);
    assert(frame->wheelRotation1 == 208);
    assert(frame->trackPointIndex1 == 210);
}

static void TestTimeAttackRecording(void) {
    GameCarRuntime player = MakeCar(300, 5);
    ReplayTimeAttackFrame untouched;
    ReplayTimeAttackFrame *frame;

    memset(&untouched, 0x5A, sizeof(untouched));
    g_ReplayFrameBuffer.timeAttackReplay[0] = untouched;
    StoreReplayTimeAttackFrame(1, &player);
    assert(memcmp(&g_ReplayFrameBuffer.timeAttackReplay[0], &untouched,
                  sizeof(untouched)) == 0);
    assert(g_ReplayPlayerModel.model == 5);

    StoreReplayTimeAttackFrame(2, &player);
    frame = &g_ReplayFrameBuffer.timeAttackReplay[1];
    assert(frame->x == 301);
    assert(frame->y == 302);
    assert(frame->modelY == 304);
    assert(frame->bodyPitch == 305);
    assert(frame->wheelRotation == 308);
    assert(frame->steeringAngle == 309);
    assert(frame->trackPointIndex == 310);
    assert(frame->tiltCounter == 311);
}

int main(void) {
    TestReplayBufferReset();
    TestGrandPrixRecording();
    TestTimeAttackRecording();
    return 0;
}
