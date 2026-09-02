#include <assert.h>

#include "game/race.h"

static s32 s_EffectVoicesEnabled;
static s32 s_ReverbLeft;
static s32 s_ReverbRight;
static s32 s_SelectBgmAssetRequests;

s32 g_SceneId;

void ForceAllEffectVoicesEnabled(s32 enabled) {
    s_EffectVoicesEnabled = enabled;
}

void SetReverbDepth(s32 left, s32 right) {
    s_ReverbLeft = left;
    s_ReverbRight = right;
}

void RequestSelectBgmAssets(void) {
    s_SelectBgmAssetRequests++;
}

static void ResetCalls(void) {
    s_EffectVoicesEnabled = -1;
    s_ReverbLeft = -1;
    s_ReverbRight = -1;
    s_SelectBgmAssetRequests = 0;
}

static void TestCommonRaceExitCleanup(void) {
    ResetCalls();

    ExitRaceScene(0x11);

    assert(g_SceneId == 0x11);
    assert(s_EffectVoicesEnabled == 0);
    assert(s_ReverbLeft == 0);
    assert(s_ReverbRight == 0);
    assert(s_SelectBgmAssetRequests == 0);
}

static void TestBgmSelectExitRequestsAssets(void) {
    ResetCalls();

    ExitRaceScene(6);

    assert(g_SceneId == 6);
    assert(s_SelectBgmAssetRequests == 1);
}

int main(void) {
    TestCommonRaceExitCleanup();
    TestBgmSelectExitRequestsAssets();
    return 0;
}
