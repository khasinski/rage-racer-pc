#include "common.h"
#include "game/audio.h"
#include "game/sound.h"
#include "psyq/snd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

EngineSoundState g_EngineSoundState;
MusicChannel g_MusicChannels[2];
EffectVoice g_EffectVoices[4];
SoundScale g_SoundScale;
s32 g_PanVoiceVolumeR;
s32 g_PanVoiceVolumeL;
s32 g_IndexedEffectIndexPrev;
s32 g_IndexedEffectIndex;
s32 g_IndexedEffectPitch;
s32 g_PanVoiceActive;
s32 g_SpecialCueVoiceA;
s32 g_SpecialCueVoiceB;
s32 g_ActiveSpecialCue;
s32 g_LastSpecialCueRequest;
s32 g_AudioLoadedSlotMask;
char g_MsgVabOpenHeadError[] = "open error";
char g_MsgVabTransBodyError[] = "body error";

static u8 s_tableArea[16];
static s32 s_playCalls[6];
static s32 s_keyOffCalls[6];
static s32 s_prepareCalls;
static s32 s_reverbOffCalls;
static s32 s_presetCalls;
static s32 s_mainVolumeCalls;
static s32 s_reservedVoiceCalls;
static s32 s_sequenceInitCalls;
static s32 s_failures;

struct SeqStruct *GetSndTableArea(void) {
    return (struct SeqStruct *)s_tableArea;
}
void PlaySoundSlotVoice(s32 slot, s32 tone, s32 vabSlot) {
    (void)tone;
    (void)vabSlot;
    s_playCalls[slot]++;
}
short SsUtKeyOffV(short voice) {
    s32 slot = voice - 14;
    if (slot >= 0 && slot < 6) s_keyOffCalls[slot]++;
    return voice;
}
void SsSetTableSize(char *table, short sequences, short tracks) {
    if (table == (char *)s_tableArea && sequences == 2 && tracks == 1) {
        s_prepareCalls++;
    }
}
void SsSetTickMode(long mode) {
    if (mode != SS_NOTICK) abort();
}
unsigned char SsSetVoiceCount(unsigned char voices) {
    if (voices != 10) abort();
    return voices;
}
void SsUtReverbOff(void) { s_reverbOffCalls++; }
void SetReverbPreset(s32 type, s32 left, s32 right) {
    if (type == 2 && left == 0 && right == 0) s_presetCalls++;
}
void SsSetMVol(short left, short right) {
    if (left == 0x3FFF && right == 0x3FFF) s_mainVolumeCalls++;
}
char SsSetReservedVoice(char voices) {
    if (voices == 0) s_reservedVoiceCalls++;
    return voices;
}
void InitSequenceAudio(void) { s_sequenceInitCalls++; }
static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

static void TestSoundSlotSwitching(void) {
    s32 slot;

    memset(&g_EngineSoundState, 0, sizeof(g_EngineSoundState));
    memset(s_playCalls, 0, sizeof(s_playCalls));
    memset(s_keyOffCalls, 0, sizeof(s_keyOffCalls));
    SetSoundSlotVoicesEnabled(1);
    for (slot = 0; slot < 5; slot++) {
        Check(g_EngineSoundState.slotActive[slot] == 1 &&
                  s_playCalls[slot] == 1,
              "automatic sound slot starts once");
    }
    Check(g_EngineSoundState.slotActive[5] == 0 && s_playCalls[5] == 0,
          "automatic switching intentionally excludes slot five");

    SetSoundSlotVoicesEnabled(1);
    for (slot = 0; slot < 5; slot++) {
        Check(s_playCalls[slot] == 1, "enabled sound slot is idempotent");
    }

    SetSoundSlotVoicesEnabled(0);
    for (slot = 0; slot < 5; slot++) {
        Check(g_EngineSoundState.slotActive[slot] == 0 &&
                  s_keyOffCalls[slot] == 1,
              "automatic sound slot stops once");
    }

}

static void TestSoundStateReset(void) {
    s32 index;

    memset(&g_EngineSoundState, 0x7F, sizeof(g_EngineSoundState));
    memset(g_MusicChannels, 0x7F, sizeof(g_MusicChannels));
    memset(g_EffectVoices, 0x7F, sizeof(g_EffectVoices));
    g_SpecialCueVoiceA = 19;
    g_SpecialCueVoiceB = 20;
    g_ActiveSpecialCue = 15;
    g_LastSpecialCueRequest = 15;
    InitSoundRuntime();

    for (index = 0; index < 6; index++) {
        Check(g_EngineSoundState.slotActive[index] == 0,
              "reset clears every sound slot");
    }
    for (index = 0; index < 2; index++) {
        Check(g_MusicChannels[index].mode == MUSIC_CHANNEL_IDLE &&
                  g_MusicChannels[index].left.value == -1 &&
                  g_MusicChannels[index].right.value == -1 &&
                  g_MusicChannels[index].volLeft == 0 &&
                  g_MusicChannels[index].volRight == 0,
              "reset initializes every music channel");
    }
    for (index = 0; index < 4; index++) {
        Check(g_EffectVoices[index].state == EFFECT_VOICE_IDLE &&
                  g_EffectVoices[index].note.value == -1 &&
                  g_EffectVoices[index].tone == -1 &&
                  g_EffectVoices[index].pitch.value == 0x1E00 &&
                  g_EffectVoices[index].volume == 0,
              "reset initializes every effect voice");
    }
    Check(g_SpecialCueVoiceA == -1 && g_SpecialCueVoiceB == -1 &&
              g_ActiveSpecialCue == -1 && g_LastSpecialCueRequest == -1,
          "reset clears special cue deduplication");
    Check(g_EngineSoundState.bank == -1 &&
              g_EngineSoundState.volumeScale == 128 &&
              g_SoundScale.scale == 128 && g_AudioLoadedSlotMask == 1,
          "reset restores sound runtime defaults");
}

static void TestRuntimeInitialization(void) {
    s_prepareCalls = 0;
    s_reverbOffCalls = 0;
    s_presetCalls = 0;
    s_mainVolumeCalls = 0;
    s_reservedVoiceCalls = 0;
    s_sequenceInitCalls = 0;
    InitSoundRuntime();
    Check(s_prepareCalls == 1 && s_reverbOffCalls == 1 && s_presetCalls == 1,
          "runtime initialization prepares libsnd");
    Check(s_mainVolumeCalls == 1 && s_reservedVoiceCalls == 1,
          "runtime initialization finishes output setup");
    Check(s_sequenceInitCalls == 1,
          "runtime initialization starts sequence audio");
}

int main(void) {
    TestSoundSlotSwitching();
    TestSoundStateReset();
    TestRuntimeInitialization();

    if (s_failures != 0) return 1;
    puts("sound runtime preserves slot ownership, reset, and initialization");
    return 0;
}
