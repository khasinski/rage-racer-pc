#include <assert.h>
#include <string.h>

#include "game/car.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/replay_internal.h"
#include "game/state.h"
#include "game/work_buffer.h"
#include "render/car_lights.h"

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
    car.brakeInput = (s16)(base % 257);
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
    s32 i;
    /* Only the assertions read it, and a release build compiles those
     * away, which leaves it set but unused. */
    (void)frame;

    memset(&untouched, 0xA5, sizeof(untouched));
    g_ReplayFrameBuffer.grandPrixReplay[0] = untouched;
    memset(g_Cars, 0, sizeof(g_Cars));
    for (i = 0; i < RACE_CAR_SLOT_COUNT; i++) {
        g_Cars[i].activeFlag = -1;
    }
    *AsRivalCar(&g_PlayerCar) = player;
    g_Cars[0] = rival;
    g_Cars[0].activeFlag = 1;
    g_Cars[0].aiEnabled = 1;
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
    assert(frame->player.brakeInput == 100);
    assert(frame->rivals[0].brakeInput == 200);
    assert(frame->rivals[4].brakeInput == 86);
    assert(frame->rivals[10].brakeInput == 0);
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
    assert(frame->rivals[10].activeFlag == -1);
    assert(frame->rivals[10].aiEnabled == 0);
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
    assert(frame->brakeInput == 43);
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

/* Exercise recording and playback together: live pedal changes must not leak
 * into a replay, and the interpolated half-frame must retain the old STOP. */
static void TestRecordedBrakeLights(s16 grandPrix, int rivalCount) {
    GameCarRuntime replayPlayer = {0};
    GameCarRuntime replayRivals[REPLAY_RIVAL_COUNT] = {{0}};
    CarLights playerLights = {0};
    CarLights rivalLights[REPLAY_RIVAL_COUNT] = {{0}};
    GameCarRuntime *player = AsRivalCar(&g_PlayerCar);

    memset(&g_ReplayFrameBuffer, 0, sizeof(g_ReplayFrameBuffer));
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    memset(g_Cars, 0, sizeof(g_Cars));
    g_GrandPrixMode = grandPrix;
    ResetReplayWriteCursor();
    for (int frame = 0; frame < 6; ++frame) {
        player->brakeInput = frame < 2 ? 256 : 0;
        for (int car = 0; car < REPLAY_RIVAL_COUNT; ++car) {
            g_Cars[car].activeFlag = car < rivalCount ? 1 : -1;
            g_Cars[car].aiEnabled = car < rivalCount;
            g_Cars[car].brakeInput = car < rivalCount && frame >= 2 ? 128 : 0;
        }
        RecordReplayFrame();
    }
    /* Deliberately contradict the recording in the live input state. */
    player->brakeInput = 256;
    memset(g_Cars, 0, sizeof(g_Cars));
    for (int frame = 0; frame < 4; ++frame) {
        ApplyReplayFrame(frame, &replayPlayer, grandPrix ? replayRivals : NULL);
        UpdateCarLights(&playerLights, 1, 1, replayPlayer.brakeInput > 0, 1.0f / 60);
        assert(playerLights.stop == (frame < 2 ? 1.0f : 0.0f));
        assert(playerLights.headlights == 0 && playerLights.tail == 0);
        if (!grandPrix) continue;
        for (int car = 0; car < REPLAY_RIVAL_COUNT; ++car) {
            UpdateCarLights(&rivalLights[car], 0, 1,
                            replayRivals[car].brakeInput > 0, 1.0f / 60);
            assert(rivalLights[car].stop ==
                   (car < rivalCount && frame >= 2 ? 1.0f : 0.0f));
            assert(replayRivals[car].activeFlag == (car < rivalCount ? 1 : -1));
            assert(replayRivals[car].aiEnabled == (car < rivalCount));
        }
    }
}

int main(void) {
    TestRecordedBrakeLights(1, REPLAY_RIVAL_COUNT);
    TestRecordedBrakeLights(1, 4);
    TestRecordedBrakeLights(0, 0);
    TestReplayBufferReset();
    TestGrandPrixRecording();
    TestTimeAttackRecording();
    TestRecordingCursorWrap();
    TestInvalidRecordingBoundsAreIgnored();
    return 0;
}
