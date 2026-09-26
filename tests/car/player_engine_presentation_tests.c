#include "game/car.h"
#include "game/car_internal.h"
#include "game/state.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

static int finished;
s32 g_AnimTimer;
s32 g_EngineRpm;
s32 g_EngineRpmJitter;
s32 g_TachoShiftLightOn;

static GameCarSpec s_spec;
u32 g_RandomSeed;
static s32 s_audioPosition;
static s32 s_audioBank;
static int s_audioCalls;
static int s_effectCalls;
static s32 s_effectIndex;
static int s_failures;

/* Reverse one LCG step to select a sample while testing the real generator. */
static void SetSample(u32 sample) {
    g_RandomSeed = (((sample << 16) - 0x3039u) * 0xEEB9EB65u) ^ (u32)g_AnimTimer;
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

static void Reset(PlayerCarRuntime *car) {
    memset(car, 0, sizeof(*car));
    memset(&s_spec, 0, sizeof(s_spec));
    s_spec.revLimit = 8000;
    s_spec.redline = 7000;
    finished = 0;
    g_AnimTimer = 0;
    g_EngineRpm = 1000;
    g_EngineRpmJitter = 99;
    g_TachoShiftLightOn = 1;
    SetSample(0);
    s_audioPosition = 0;
    s_audioBank = 0;
    s_audioCalls = 0;
    s_effectCalls = 0;
    s_effectIndex = 0;
}

#define CHECK(condition) do {                                                \
    if (!(condition)) {                                                      \
        printf("FAIL line %d: %s\n", __LINE__, #condition);                 \
        s_failures++;                                                        \
    }                                                                        \
} while (0)

static void PresentEngine(PlayerCarRuntime *car) {
    const PlayerCarRuntime before = *car;
    const u32 seed = g_RandomSeed;
    UpdatePlayerEnginePresentation(car, &s_spec, finished);
    CHECK(memcmp(car, &before, sizeof(before)) == 0);
    CHECK(g_RandomSeed == seed);
}

int main(void) {
    PlayerCarRuntime car;

    Reset(&car);
    car.drive.engineRpm = 4500;
    car.drive.gear = 1;
    car.drive.gearDisp = 99;
    car.drive.acceleratorInput.value = 256;
    PresentEngine(&car);
    CHECK(g_EngineRpm == 1875);
    CHECK(s_audioCalls == 1 && s_audioPosition == 1875 && s_audioBank == 1);
    CHECK(car.drive.gearDisp == 99);

    Reset(&car);
    car.drive.engineRpm = 4500;
    car.drive.clutch = 1;
    car.drive.gear = 1;
    car.drive.manual = 1;
    car.drive.acceleratorInput.value = 256;
    PresentEngine(&car);
    CHECK(g_EngineRpm == 2750);
    CHECK(s_audioBank == 0);

    Reset(&car);
    g_EngineRpm = 7900;
    g_AnimTimer = 2;
    SetSample(149);
    car.drive.engineRpm = 10000;
    car.drive.clutch = 1;
    car.drive.gear = 2;
    car.drive.acceleratorInput.value = 256;
    PresentEngine(&car);
    CHECK(g_EngineRpm == 8000 && g_EngineRpmJitter == 74);
    CHECK(g_TachoShiftLightOn == 1 && s_audioPosition == 8074);
    CHECK(s_audioBank == 1);

    Reset(&car);
    g_EngineRpm = 0;
    g_AnimTimer = 8;
    SetSample(0x400);
    car.drive.engineRpm = 0;
    PresentEngine(&car);
    CHECK(g_EngineRpm == 500 && g_EngineRpmJitter == 150);
    CHECK(s_audioPosition == 650 && s_audioBank == 0);

    Reset(&car);
    g_EngineRpm = 0;
    g_AnimTimer = 8;
    SetSample(0x200);
    car.drive.engineRpm = 0;
    PresentEngine(&car);
    CHECK(g_EngineRpmJitter == 106 && s_audioPosition == 606);

    Reset(&car);
    g_EngineRpm = 0;
    g_AnimTimer = 8;
    SetSample(0xC00);
    car.drive.engineRpm = 0;
    PresentEngine(&car);
    CHECK(g_EngineRpmJitter == 0 && s_audioPosition == 500);

    Reset(&car);
    finished = 1;
    car.drive.engineRpm = 1000;
    car.drive.gear = 1;
    PresentEngine(&car);
    CHECK(s_effectCalls == 1 && s_effectIndex == -1);

    Reset(&car);
    g_EngineRpm = INT_MIN;
    car.drive.engineRpm = 0;
    PresentEngine(&car);
    CHECK(g_EngineRpm == 8000);
    CHECK(s_audioCalls == 1 && s_audioPosition == 8000);

    if (s_failures != 0) {
        printf("%d player engine presentation checks failed\n", s_failures);
        return 1;
    }
    puts("player engine audio and displayed rpm are bounded");
    return 0;
}
