#include <assert.h>
#include <string.h>

#include "game/car.h"
#include "game/player_car_internal.h"
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
s16 g_ReplayPlayerModelIndex;
s16 g_ReplayRivalModelIndex;
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
    BindReplayFrameBuffers();
    assert(g_ReplayFramesGp == g_ReplayFrameBuffer.grandPrixReplay);
    assert(g_ReplayFramesTimeAttack == g_ReplayFrameBuffer.timeAttackReplay);

    g_GrandPrixMode = 1;
    g_ReplayWriteCursor = 99;
    g_ReplayBufferWrapped = 1;
    ResetReplayWriteCursor();
    assert(g_ReplayWriteCursor == 0);
    assert(g_ReplayFrameCount == GRAND_PRIX_REPLAY_SUBFRAME_COUNT);
    assert(g_ReplayBufferWrapped == 0);

    g_GrandPrixMode = 0;
    ResetReplayWriteCursor();
    assert(g_ReplayFrameCount == TIME_ATTACK_REPLAY_SUBFRAME_COUNT);
}

static void TestGrandPrixRecording(void) {
    GameCarRuntime player = MakeCar(100, 3);
    GameCarRuntime rival = MakeCar(200, 4);
    ReplayGrandPrixFrame untouched;
    ReplayGrandPrixFrame *frame;
    /* Only the assertions read it, and a release build compiles those
     * away, which leaves it set but unused. */
    (void)frame;

    BindReplayFrameBuffers();
    memset(&untouched, 0xA5, sizeof(untouched));
    g_ReplayFrameBuffer.grandPrixReplay[0] = untouched;
    *AsRivalCar(&g_PlayerCar) = player;
    g_Cars[0] = rival;
    g_GrandPrixMode = 1;
    g_ReplayWriteCursor = 1;
    g_ReplayFrameCount = GRAND_PRIX_REPLAY_SUBFRAME_COUNT;
    RecordReplayFrame();
    assert(memcmp(&g_ReplayFrameBuffer.grandPrixReplay[0], &untouched,
                  sizeof(untouched)) == 0);
    assert(g_ReplayPlayerModelIndex == 3);
    assert(g_ReplayRivalModelIndex == 4);

    assert(g_ReplayWriteCursor == 2);
    RecordReplayFrame();
    frame = &g_ReplayFrameBuffer.grandPrixReplay[1];
    assert(frame->x0 == 101);
    assert(frame->y0 == 102);
    assert(frame->z0 == 103);
    assert(frame->modelY0 == 104);
    assert(frame->bodyPitch0 == 105);
    assert(frame->bodyYaw0 == 106);
    assert(frame->bodyRoll0 == 107);
    assert(frame->wheelRotation0 == 108);
    assert(frame->steeringAngle0 == 109);
    assert(frame->trackPointIndex0 == 110);
    assert(frame->tiltCounter == 111);
    assert(frame->x1 == 201);
    assert(frame->y1 == 202);
    assert(frame->z1 == 203);
    assert(frame->modelY1 == 204);
    assert(frame->bodyPitch1 == 205);
    assert(frame->bodyYaw1 == 206);
    assert(frame->bodyRoll1 == 207);
    assert(frame->wheelRotation1 == 208);
    assert(frame->steeringAngle1 == 209);
    assert(frame->trackPointIndex1 == 210);
}

static void TestTimeAttackRecording(void) {
    GameCarRuntime player = MakeCar(300, 5);
    ReplayTimeAttackFrame untouched;
    ReplayTimeAttackFrame *frame;
    /* Only the assertions read it, and a release build compiles those
     * away, which leaves it set but unused. */
    (void)frame;

    BindReplayFrameBuffers();
    memset(&untouched, 0x5A, sizeof(untouched));
    g_ReplayFrameBuffer.timeAttackReplay[0] = untouched;
    *AsRivalCar(&g_PlayerCar) = player;
    g_GrandPrixMode = 0;
    g_ReplayWriteCursor = 1;
    g_ReplayFrameCount = TIME_ATTACK_REPLAY_SUBFRAME_COUNT;
    RecordReplayFrame();
    assert(memcmp(&g_ReplayFrameBuffer.timeAttackReplay[0], &untouched,
                  sizeof(untouched)) == 0);
    assert(g_ReplayPlayerModelIndex == 5);

    assert(g_ReplayWriteCursor == 2);
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
    g_ReplayWriteCursor = 1;
    g_ReplayFrameCount = 2;
    g_ReplayBufferWrapped = 0;

    RecordReplayFrame();
    assert(g_ReplayWriteCursor == 0);
    assert(g_ReplayBufferWrapped == 1);
}

int main(void) {
    TestReplayBufferReset();
    TestGrandPrixRecording();
    TestTimeAttackRecording();
    TestRecordingCursorWrap();
    return 0;
}
