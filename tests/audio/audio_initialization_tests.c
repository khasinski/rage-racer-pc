#include "common.h"
#include "game/audio.h"
#include "game/audio_internal.h"
#include "game/sound.h"
#include "psyq/snd.h"

#include <stdio.h>
#include <string.h>

MusicChannel g_MusicChannels[2];
EffectVoice g_EffectVoices[4];
s32 g_PanVoiceVolumeR;
s32 g_PanVoiceVolumeL;
s32 g_IndexedEffectIndexPrev;
s32 g_IndexedEffectIndex;
s32 g_IndexedEffectPitch;
s32 g_PanVoiceActive;
s32 g_ActiveSpecialCue;
s32 g_LastSpecialCueRequest;
s32 g_ReverbFadeStep;
s32 g_CarSoundVolumeScales[4];
s32 g_PlayerCarIndex;

static s32 s_vmInitCalls;
static s32 s_voiceCount;
static s32 s_reverbLeft;
static s32 s_reverbRight;
static s32 s_refreshCalls;
static s32 s_slotEnable;
static s32 s_presetType;
static s32 s_loadedScale;
static s32 s_failures;

void _SsVmInit(int voices) {
    if (voices == 0) s_vmInitCalls++;
}
char SsSetReservedVoice(char voices) {
    s_voiceCount = voices;
    return voices;
}
void SetDefaultReverbDepth(void) {
    s_reverbLeft = 40;
    s_reverbRight = 40;
}
void RefreshSequenceVolumeScale(void) { s_refreshCalls++; }
void SetSoundSlotVoicesEnabled(s32 enabled) { s_slotEnable = enabled; }
void SetReverbPreset(s32 type, s32 left, s32 right) {
    (void)left;
    (void)right;
    s_presetType = type;
}
void SetLoadedTableVolumeScale(s32 scale) { s_loadedScale = scale; }
s32 GetOwnedCarAssetIndex(s32 model) { return model + 1; }

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

static void TestSequenceInitialization(void) {
    g_ReverbFadeStep = -3;
    s_vmInitCalls = 0;
    InitSequenceAudio();
    Check(s_vmInitCalls == 1 && s_voiceCount == 18,
          "sequence initialization resets libsnd with eighteen voices");
    Check(s_reverbLeft == 40 && s_reverbRight == 40 &&
              g_ReverbFadeStep == 0 && s_refreshCalls == 1,
          "sequence initialization restores reverb and saved volume");
}

static void TestEffectInitialization(void) {
    s32 index;

    memset(g_MusicChannels, 0x7F, sizeof(g_MusicChannels));
    memset(g_EffectVoices, 0x7F, sizeof(g_EffectVoices));
    g_ActiveSpecialCue = 15;
    g_LastSpecialCueRequest = 15;
    g_PlayerCarIndex = 2;
    g_CarSoundVolumeScales[3] = 91;
    s_vmInitCalls = 0;
    InitEffectVoiceRuntime();

    Check(s_vmInitCalls == 1 && s_voiceCount == 8,
          "effect initialization resets libsnd with eight voices");
    for (index = 0; index < 2; index++) {
        Check(g_MusicChannels[index].mode == MUSIC_CHANNEL_IDLE &&
                  g_MusicChannels[index].volLeft == 0 &&
                  g_MusicChannels[index].volRight == 0,
              "effect initialization resets music channel state");
    }
    for (index = 0; index < 4; index++) {
        Check(g_EffectVoices[index].state == EFFECT_VOICE_IDLE &&
                  g_EffectVoices[index].pitch.value == 0x1E00,
              "effect initialization resets effect voice state");
    }
    Check(g_ActiveSpecialCue == -1 && g_LastSpecialCueRequest == -1,
          "effect initialization clears special cue deduplication");
    Check(s_slotEnable == 1 && s_presetType == 2 && s_loadedScale == 91,
          "effect initialization enables slots and selects car volume");

    g_PlayerCarIndex = -2;
    g_CarSoundVolumeScales[0] = 37;
    InitEffectVoiceRuntime();
    Check(s_loadedScale == 37,
          "invalid car asset uses the base volume scale");
}

int main(void) {
    TestSequenceInitialization();
    TestEffectInitialization();

    if (s_failures != 0) return 1;
    puts("audio initialization shares voice reset and applies mode settings");
    return 0;
}
