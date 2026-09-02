#include "common.h"
#include "game/audio.h"
#include "game/sound.h"

#include <stdio.h>
#include <string.h>

EngineSoundState g_EngineSoundState;
s16 g_SoundSlotTone[ENGINE_SOUND_SLOT_COUNT][2];

static s32 s_slotEnableCalls;
static s32 s_lastSlotEnable;
static s32 s_playCalls;
static s32 s_playSlot;
static s32 s_playTone;
static s32 s_playVab;
static s32 s_interpolateCalls;
static s32 s_interpolateParams[8];
static s32 s_setToneCalls;
static s32 s_setToneSlot[4];
static s32 s_setToneBend[4];
static s32 s_setToneVolume[4];
static s32 s_setToneBank[4];
static s32 s_setToneVab[4];
static s32 s_forceOrder[4];
static s32 s_forceCalls;

void SetSoundSlotVoicesEnabled(s32 enabled) {
    s_slotEnableCalls++;
    s_lastSlotEnable = enabled;
}

void PlaySoundSlotVoice(s32 slot, s32 tone, s32 vabSlot) {
    s_playCalls++;
    s_playSlot = slot;
    s_playTone = tone;
    s_playVab = vabSlot;
}

s32 InterpolateAudioParameter(s32 param, s32 position, s32 bank) {
    s_interpolateParams[s_interpolateCalls++] = param;
    return param * 10 + position + bank;
}

void SetSoundSlotTone(s32 slot, s32 bend, s32 volume, s32 bank, u16 vabSlot) {
    s_setToneSlot[s_setToneCalls] = slot;
    s_setToneBend[s_setToneCalls] = bend;
    s_setToneVolume[s_setToneCalls] = volume;
    s_setToneBank[s_setToneCalls] = bank;
    s_setToneVab[s_setToneCalls] = vabSlot;
    s_setToneCalls++;
}

void ForcePanVoiceEnabled(s32 enabled) {
    s_forceOrder[s_forceCalls++] = 10 + enabled;
}

void ForceBasicEffectVoicesEnabled(s32 enabled) {
    s_forceOrder[s_forceCalls++] = 20 + enabled;
}

void ForceIndexedEffectVoiceEnabled(s32 enabled) {
    s_forceOrder[s_forceCalls++] = 30 + enabled;
}

void ForcePitchEffectVoicesEnabled(s32 enabled) {
    s_forceOrder[s_forceCalls++] = 40 + enabled;
}

#define CHECK(condition) do {                                                   \
    if (!(condition)) {                                                         \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition); \
        return 1;                                                               \
    }                                                                           \
} while (0)

static void Reset(void) {
    memset(&g_EngineSoundState, 0, sizeof(g_EngineSoundState));
    memset(g_SoundSlotTone, 0, sizeof(g_SoundSlotTone));
    s_slotEnableCalls = 0;
    s_lastSlotEnable = -1;
    s_playCalls = 0;
    s_interpolateCalls = 0;
    s_setToneCalls = 0;
    s_forceCalls = 0;
}

int main(void) {
    Reset();
    ForceSoundSlotVoicePlayback(0);
    CHECK(s_slotEnableCalls == 1 && s_lastSlotEnable == 0);
    CHECK(s_playCalls == 0 && s_interpolateCalls == 0 && s_setToneCalls == 0);

    Reset();
    g_EngineSoundState.position = 100;
    g_EngineSoundState.bank = 1;
    g_EngineSoundState.volumeScale = 64;
    g_EngineSoundState.slotActive[1] = 1;
    g_EngineSoundState.slotActive[3] = 1;
    g_SoundSlotTone[1][0] = 5;
    g_SoundSlotTone[1][1] = 6;
    g_SoundSlotTone[3][0] = 9;
    g_SoundSlotTone[3][1] = 9;
    ForceSoundSlotVoicePlayback(1);
    CHECK(s_slotEnableCalls == 1 && s_lastSlotEnable == 1);
    CHECK(s_playCalls == 1 && s_playSlot == 1);
    CHECK(s_playTone == 1 && s_playVab == 3);
    CHECK(s_interpolateCalls == 4);
    CHECK(s_interpolateParams[0] == 2 && s_interpolateParams[1] == 3);
    CHECK(s_interpolateParams[2] == 6 && s_interpolateParams[3] == 7);
    CHECK(s_setToneCalls == 2);
    CHECK(s_setToneSlot[0] == 1 && s_setToneBend[0] == 121);
    CHECK(s_setToneVolume[0] == 65);
    CHECK(s_setToneSlot[1] == 3 && s_setToneBend[1] == 161);
    CHECK(s_setToneVolume[1] == 85);
    CHECK(s_setToneBank[0] == 1 && s_setToneVab[0] == 3);

    Reset();
    ForceAllEffectVoicesEnabled(1);
    CHECK(s_forceCalls == 4);
    CHECK(s_forceOrder[0] == 11 && s_forceOrder[1] == 21);
    CHECK(s_forceOrder[2] == 31 && s_forceOrder[3] == 41);
    CHECK(s_slotEnableCalls == 1 && s_lastSlotEnable == 1);

    puts("forced voice playback restores active engine slots and all families");
    return 0;
}
