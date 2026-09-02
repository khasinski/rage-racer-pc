#include <assert.h>

#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/replay_internal.h"

typedef enum ReplayCarCall {
    CALL_INIT_SCENERY,
    CALL_APPLY_FRAME,
    CALL_FIND_SEGMENT,
    CALL_SEED_PROGRESS,
    CALL_ACCUMULATE_PROGRESS,
    CALL_RESET_TRACK_STATE,
    CALL_REQUEST_TEXTURE,
} ReplayCarCall;

typedef struct CallRecord {
    ReplayCarCall call;
    GameCarRuntime *car;
    s32 value;
} CallRecord;

static CallRecord s_Calls[16];
static s32 s_CallCount;

PlayerCarRuntime g_PlayerCar;
GameCarRuntime g_Cars[RACE_CAR_SLOT_COUNT];
s16 g_GrandPrixMode;
s32 g_ReplayReadCursor;

static void RecordCall(ReplayCarCall call, GameCarRuntime *car, s32 value) {
    assert(s_CallCount < (s32)(sizeof(s_Calls) / sizeof(s_Calls[0])));
    s_Calls[s_CallCount++] = (CallRecord){call, car, value};
}

void InitShuttleScenery(void) {
    RecordCall(CALL_INIT_SCENERY, NULL, 0);
}

void ApplyReplayFrameAndTrackPoint(s32 subframe, GameCarRuntime *player,
                                   GameCarRuntime *rival) {
    RecordCall(CALL_APPLY_FRAME, player, subframe);
    assert(rival == &g_Cars[0]);
}

s32 FindTrackSegment(GameCarRuntime *car, s32 index) {
    RecordCall(CALL_FIND_SEGMENT, car, index);
    return index + 100;
}

void SeedCarLapProgress(GameCarRuntime *car, s32 mode) {
    RecordCall(CALL_SEED_PROGRESS, car, mode);
}

void AccumulateLapProgress(GameCarRuntime *car) {
    RecordCall(CALL_ACCUMULATE_PROGRESS, car, 0);
}

void ResetCarTrackState(GameCarRuntime *car) {
    RecordCall(CALL_RESET_TRACK_STATE, car, 0);
}

void RequestTrackTexturePage(s32 trackSection) {
    RecordCall(CALL_REQUEST_TEXTURE, NULL, trackSection);
}

static void ExpectCall(s32 index, ReplayCarCall call, GameCarRuntime *car,
                       s32 value) {
    assert(s_Calls[index].call == call);
    assert(s_Calls[index].car == car);
    assert(s_Calls[index].value == value);
}

static void TestSeedTimeAttackCar(void) {
    GameCarRuntime *player = AsRivalCar(&g_PlayerCar);

    s_CallCount = 0;
    g_GrandPrixMode = 0;
    g_ReplayReadCursor = 42;
    player->trackPointIndex = 7;

    SeedReplayCars();

    assert(s_CallCount == 6);
    ExpectCall(0, CALL_INIT_SCENERY, NULL, 0);
    ExpectCall(1, CALL_APPLY_FRAME, player, 42);
    ExpectCall(2, CALL_FIND_SEGMENT, player, 7);
    ExpectCall(3, CALL_SEED_PROGRESS, player, 1);
    ExpectCall(4, CALL_ACCUMULATE_PROGRESS, player, 0);
    ExpectCall(5, CALL_RESET_TRACK_STATE, player, 0);
    assert(player->trackPointIndex == 107);
}

static void TestSeedGrandPrixCarsForAnyNonzeroMode(void) {
    GameCarRuntime *player = AsRivalCar(&g_PlayerCar);
    GameCarRuntime *rival = &g_Cars[0];

    s_CallCount = 0;
    g_GrandPrixMode = 2;
    g_ReplayReadCursor = 9;
    player->trackPointIndex = 3;
    rival->trackPointIndex = 5;

    SeedReplayCars();

    assert(s_CallCount == 10);
    ExpectCall(2, CALL_FIND_SEGMENT, player, 3);
    ExpectCall(6, CALL_FIND_SEGMENT, rival, 5);
    ExpectCall(7, CALL_SEED_PROGRESS, rival, 1);
    ExpectCall(8, CALL_ACCUMULATE_PROGRESS, rival, 0);
    ExpectCall(9, CALL_RESET_TRACK_STATE, rival, 0);
    assert(player->trackPointIndex == 103);
    assert(rival->trackPointIndex == 105);
}

static void TestUpdateCarsAndRequestPlayerTexture(void) {
    GameCarRuntime *player = AsRivalCar(&g_PlayerCar);
    GameCarRuntime *rival = &g_Cars[0];

    s_CallCount = 0;
    g_GrandPrixMode = 1;
    player->trackSection = 27;

    UpdateReplayCars();

    assert(s_CallCount == 5);
    ExpectCall(0, CALL_ACCUMULATE_PROGRESS, player, 0);
    ExpectCall(1, CALL_RESET_TRACK_STATE, player, 0);
    ExpectCall(2, CALL_ACCUMULATE_PROGRESS, rival, 0);
    ExpectCall(3, CALL_RESET_TRACK_STATE, rival, 0);
    ExpectCall(4, CALL_REQUEST_TEXTURE, NULL, 27);
}

int main(void) {
    TestSeedTimeAttackCar();
    TestSeedGrandPrixCarsForAnyNonzeroMode();
    TestUpdateCarsAndRequestPlayerTexture();
    return 0;
}
