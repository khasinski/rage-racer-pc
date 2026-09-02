#include "common.h"
#include "game/audio.h"
#include "game/sound.h"

#include <stdio.h>
#include <string.h>

EngineSoundState g_EngineSoundState;
s16 g_SoundSlotTone[ENGINE_SOUND_SLOT_COUNT][ENGINE_SOUND_BANK_COUNT];

static s32 s_playCalls;
static s32 s_playSlot;
static s32 s_playBank;
static s32 s_interpolateCalls;
static s32 s_interpolateParam[8];
static s32 s_interpolatePosition[8];
static s32 s_interpolateBank[8];
static s32 s_toneCalls;
static s32 s_toneSlot[4];
static s32 s_toneBend[4];
static s32 s_toneVolume[4];
static s32 s_tailCalls;
static s32 s_slotsEnabled;

void PlaySoundSlotVoice(s32 slot, s32 tone, s32 vabSlot) {
    (void)vabSlot;
    s_playCalls++;
    s_playSlot = slot;
    s_playBank = tone;
}

s32 InterpolateAudioParameter(s32 param, s32 position, s32 bank) {
    s_interpolateParam[s_interpolateCalls] = param;
    s_interpolatePosition[s_interpolateCalls] = position;
    s_interpolateBank[s_interpolateCalls] = bank;
    s_interpolateCalls++;
    return param * 10 + 5;
}

void SetSoundSlotTone(s32 slot, s32 bend, s32 volume, s32 toneIndex,
                      u16 vabSlot) {
    (void)toneIndex;
    (void)vabSlot;
    s_toneSlot[s_toneCalls] = slot;
    s_toneBend[s_toneCalls] = bend;
    s_toneVolume[s_toneCalls] = volume;
    s_toneCalls++;
}

void ApplyPanVoiceVolume(void) { s_tailCalls++; }
void UpdateBasicEffectVoices(void) { s_tailCalls++; }
void UpdateIndexedEffectVoice(void) { s_tailCalls++; }
void UpdateEffectVoiceStates(void) { s_tailCalls++; }
void SetSoundSlotVoicesEnabled(s32 enabled) { s_slotsEnabled = enabled; }

#define CHECK(condition) do {                                                   \
    if (!(condition)) {                                                         \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition); \
        return 1;                                                               \
    }                                                                           \
} while (0)

int main(void) {
    memset(&g_EngineSoundState, 0, sizeof(g_EngineSoundState));
    memset(g_SoundSlotTone, 0, sizeof(g_SoundSlotTone));
    g_EngineSoundState.maxRpm = 10000;
    g_EngineSoundState.bank = 0;
    g_EngineSoundState.slotActive[0] = 1;
    g_EngineSoundState.slotActive[2] = 1;
    g_EngineSoundState.slotActive[5] = 1;
    g_EngineSoundState.volumeScale = 64;
    g_SoundSlotTone[0][0] = 10;
    g_SoundSlotTone[0][1] = 11;
    g_SoundSlotTone[2][0] = 20;
    g_SoundSlotTone[2][1] = 20;
    g_SoundSlotTone[5][0] = 30;
    g_SoundSlotTone[5][1] = 31;

    UpdateLoadedAudioVoices(5000, 1);
    CHECK(g_EngineSoundState.position == 5120);
    CHECK(g_EngineSoundState.bank == 1);
    CHECK(s_playCalls == 2 && s_playSlot == 5 && s_playBank == 1);
    CHECK(s_interpolateCalls == 6);
    CHECK(s_interpolateParam[0] == 0 && s_interpolateParam[1] == 1);
    CHECK(s_interpolateParam[2] == 4 && s_interpolateParam[3] == 5);
    CHECK(s_interpolatePosition[0] == 5120 && s_interpolateBank[0] == 1);
    CHECK(s_interpolateParam[4] == 10 && s_interpolateParam[5] == 11);
    CHECK(s_toneCalls == 3);
    CHECK(s_toneSlot[0] == 0 && s_toneBend[0] == 5 && s_toneVolume[0] == 7);
    CHECK(s_toneSlot[1] == 2 && s_toneBend[1] == 45 && s_toneVolume[1] == 27);
    CHECK(s_toneSlot[2] == 5 && s_toneBend[2] == 105 &&
          s_toneVolume[2] == 57);
    CHECK(s_tailCalls == 4);

    s_interpolateCalls = 0;
    s_toneCalls = 0;
    UpdateLoadedAudioVoices(2500, 1);
    CHECK(g_EngineSoundState.position == 2560);
    CHECK(s_playCalls == 2);
    CHECK(s_interpolateCalls == 6 && s_toneCalls == 3);
    CHECK(s_tailCalls == 8);

    s_interpolateCalls = 0;
    s_toneCalls = 0;
    s_playCalls = 0;
    ForceSoundSlotVoicePlayback(1);
    CHECK(s_slotsEnabled == 1 && s_playCalls == 2);
    CHECK(s_interpolateCalls == 6 && s_toneCalls == 3);
    CHECK(s_interpolatePosition[0] == 2560 && s_interpolateBank[0] == 1);

    s_interpolateCalls = 0;
    s_toneCalls = 0;
    ForceSoundSlotVoicePlayback(0);
    CHECK(s_slotsEnabled == 0 && s_interpolateCalls == 0 && s_toneCalls == 0);

    memset(g_EngineSoundState.slotActive, 0,
           sizeof(g_EngineSoundState.slotActive));
    g_EngineSoundState.maxRpm = 0;
    UpdateLoadedAudioVoices(5000, -10);
    CHECK(g_EngineSoundState.position == 0 && g_EngineSoundState.bank == 0);
    UpdateLoadedAudioVoices(5000, ENGINE_SOUND_BANK_COUNT + 10);
    CHECK(g_EngineSoundState.position == 0 &&
          g_EngineSoundState.bank == ENGINE_SOUND_BANK_COUNT - 1);

    puts("engine sound runtime preserves slot routing, scaling, and updates");
    return 0;
}
