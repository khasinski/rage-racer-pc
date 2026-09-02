#include <stdio.h>

#include "common.h"
#include "game/car.h"
#include "game/race.h"

GameCarRuntime g_Cars[RACE_CAR_SLOT_COUNT];
s32 g_SceneTimer;
s32 g_TrackTextureCursorRow;

static s32 s_random;

s32 Random15(void) {
    return s_random;
}

s32 TrackTexturePageForSection(s32 section) {
    return section >= 10 && section < 20 ? 0x100 : 0;
}

static int s_failures;

static void Check(int condition, const char *message) {
    if (condition) return;
    s_failures++;
    printf("FAIL %s\n", message);
}

static void InvalidCurrentCarReturnsToTheFirstCar(void) {
    g_SceneTimer = 1;

    Check(CycleAttractCameraCar(0xFF, -1) == 0,
          "negative attract camera index resets");
    Check(CycleAttractCameraCar(0xFF, 4) == 0,
          "past-end attract camera index resets");
    Check(CycleBgmSelectCameraCar(0xFF, RACE_CAR_SLOT_COUNT) == 0,
          "past-end BGM camera index resets");
}

static void TimerMaskControlsWhenTheCameraChanges(void) {
    g_TrackTextureCursorRow = 0;
    g_Cars[1].trackSection = 2;
    g_Cars[2].trackSection = 3;
    s_random = 2;

    g_SceneTimer = 2;
    Check(CycleAttractCameraCar(2, 1) == 1,
          "matching timer bit keeps the current car");
    Check(CycleAttractCameraCar(1, 1) == 2,
          "clear timer bit permits a camera change");
}

static void CameraOnlyChangesAtACompletedTexturePage(void) {
    g_SceneTimer = 0;
    g_Cars[0].trackSection = 2;
    g_Cars[2].trackSection = 3;
    s_random = 2;

    g_TrackTextureCursorRow = 1;
    Check(CycleAttractCameraCar(0, 0) == 0,
          "camera waits while texture rows are changing");
    g_TrackTextureCursorRow = 0x100;
    Check(CycleAttractCameraCar(0, 0) == 2,
          "second completed texture page permits a camera change");
}

static void CandidateMustUseTheCurrentTexturePage(void) {
    g_SceneTimer = 0;
    g_TrackTextureCursorRow = 0;
    g_Cars[0].trackSection = 2;
    g_Cars[2].trackSection = 12;
    s_random = 2;

    Check(CycleAttractCameraCar(0, 0) == 0,
          "candidate on another texture page is rejected");

    g_Cars[0].trackSection = 12;
    Check(CycleAttractCameraCar(0, 0) == 2,
          "candidate on the same texture page is accepted");
}

int main(void) {
    InvalidCurrentCarReturnsToTheFirstCar();
    TimerMaskControlsWhenTheCameraChanges();
    CameraOnlyChangesAtACompletedTexturePage();
    CandidateMustUseTheCurrentTexturePage();

    if (s_failures != 0) {
        printf("%d camera car selection checks failed\n", s_failures);
        return 1;
    }
    printf("camera car selection respects indices, timing and texture pages\n");
    return 0;
}
