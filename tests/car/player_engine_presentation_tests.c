#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/state.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

GameCarSpec *g_CarSpec;
s16 g_RacePhase;
s32 g_AnimTimer;
s32 g_EngineRpm;
s32 g_EngineRpmJitter;
s32 g_EngineRpmSnapshot;
s32 g_TachoShiftLightOn;

static GameCarSpec s_spec;
static s32 s_randomValue;
static s32 s_audioPosition;
static s32 s_audioBank;
static int s_audioCalls;
static int s_effectCalls;
static s32 s_effectIndex;
static int s_traceCalls;
static int s_failures;

s32 Random15(void) {
    return s_randomValue;
}

s32 rsin(s32 angle) {
    return angle == 0x400 ? 0x1000 : 0;
}

void UpdateLoadedAudioVoices(s32 position, s32 bank) {
    s_audioPosition = position;
    s_audioBank = bank;
    s_audioCalls++;
}

void SetIndexedEffectVoice(s32 index, s32 phase, s32 volume) {
    (void)phase;
    (void)volume;
    s_effectIndex = index;
    s_effectCalls++;
}

void TraceCarMotion(const char *phase, PlayerCarRuntime *car) {
    (void)phase;
    (void)car;
    s_traceCalls++;
}

static void Reset(PlayerCarRuntime *car) {
    memset(car, 0, sizeof(*car));
    memset(&s_spec, 0, sizeof(s_spec));
    s_spec.revLimit = 8000;
    s_spec.redline = 7000;
    g_CarSpec = &s_spec;
    g_RacePhase = 2;
    g_AnimTimer = 0;
    g_EngineRpm = 1000;
    g_EngineRpmJitter = 99;
    g_EngineRpmSnapshot = 0;
    g_TachoShiftLightOn = 1;
    s_randomValue = 0;
    s_audioPosition = 0;
    s_audioBank = 0;
    s_audioCalls = 0;
    s_effectCalls = 0;
    s_effectIndex = 0;
    s_traceCalls = 0;
}

#define CHECK(condition) do {                                                \
    if (!(condition)) {                                                      \
        printf("FAIL line %d: %s\n", __LINE__, #condition);                 \
        s_failures++;                                                        \
    }                                                                        \
} while (0)

int main(void) {
    PlayerCarRuntime car;

    Reset(&car);
    car.drive.engineRpm = 4500;
    car.drive.gear = 1;
    car.drive.acceleratorInput.value = 256;
    UpdatePlayerEnginePresentation(&car);
    CHECK(g_EngineRpm == 1875 && g_EngineRpmSnapshot == 1875);
    CHECK(s_audioCalls == 1 && s_audioPosition == 1875 && s_audioBank == 1);
    CHECK(car.drive.gearDisp == 1 && s_traceCalls == 1);

    Reset(&car);
    car.drive.engineRpm = 4500;
    car.drive.clutch = 1;
    car.drive.gear = 1;
    car.drive.manual = 1;
    car.drive.acceleratorInput.value = 256;
    UpdatePlayerEnginePresentation(&car);
    CHECK(g_EngineRpm == 2750);
    CHECK(s_audioBank == 0);

    Reset(&car);
    g_EngineRpm = 7900;
    g_AnimTimer = 2;
    s_randomValue = 149;
    car.drive.engineRpm = 10000;
    car.drive.clutch = 1;
    car.drive.gear = 2;
    car.drive.acceleratorInput.value = 256;
    UpdatePlayerEnginePresentation(&car);
    CHECK(g_EngineRpm == 8000 && g_EngineRpmJitter == 74);
    CHECK(g_TachoShiftLightOn == 1 && s_audioPosition == 8074);
    CHECK(s_audioBank == 1);

    Reset(&car);
    g_EngineRpm = 0;
    g_AnimTimer = 8;
    s_randomValue = 0x400;
    car.drive.engineRpm = 0;
    UpdatePlayerEnginePresentation(&car);
    CHECK(g_EngineRpm == 500 && g_EngineRpmJitter == 150);
    CHECK(s_audioPosition == 650 && s_audioBank == 0);

    Reset(&car);
    g_RacePhase = 4;
    car.drive.engineRpm = 1000;
    car.drive.gear = 1;
    UpdatePlayerEnginePresentation(&car);
    CHECK(s_effectCalls == 1 && s_effectIndex == -1);

    Reset(&car);
    g_EngineRpm = INT_MIN;
    car.drive.engineRpm = 0;
    UpdatePlayerEnginePresentation(&car);
    CHECK(g_EngineRpm == 8000 && g_EngineRpmSnapshot == 8000);
    CHECK(s_audioCalls == 1 && s_audioPosition == 8000);

    if (s_failures != 0) {
        printf("%d player engine presentation checks failed\n", s_failures);
        return 1;
    }
    puts("player engine audio and displayed rpm are bounded");
    return 0;
}
