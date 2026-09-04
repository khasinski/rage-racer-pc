#include "common.h"
#include "game/audio.h"
#include "game/audio_internal.h"
#include "game/sound.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

SoundScale g_SoundScale;
MusicChannel g_MusicChannels[AUDIO_MUSIC_CHANNEL_COUNT];
IndexedEffect g_IndexedEffects[AUDIO_INDEXED_EFFECT_COUNT];
s32 g_StereoOutput;
s32 g_PanVoiceActive;
s32 g_PanVoiceVolumeL;
s32 g_PanVoiceVolumeR;
s32 g_IndexedEffectIndex;
s32 g_IndexedEffectIndexPrev;
s32 g_IndexedEffectPitch;
s32 g_IndexedEffectVolume;

typedef struct VoiceCall {
    s16 voice;
    s16 program;
    s16 tone;
    s16 left;
    s16 right;
    s16 note;
    s16 fine;
} VoiceCall;

static VoiceCall s_keyOn[8];
static VoiceCall s_keyOff[8];
static VoiceCall s_volume[8];
static VoiceCall s_pitch[8];
static s32 s_keyOnCount;
static s32 s_keyOffCount;
static s32 s_volumeCount;
static s32 s_pitchCount;

short SsUtKeyOnV(short voice, short vabId, short program, short tone,
                 short note, short fine, short left, short right) {
    VoiceCall *call = &s_keyOn[s_keyOnCount++];
    (void)vabId;
    call->voice = voice;
    call->program = program;
    call->tone = tone;
    call->note = note;
    call->fine = fine;
    call->left = left;
    call->right = right;
    return voice;
}

short SsUtKeyOffV(short voice) {
    s_keyOff[s_keyOffCount++].voice = voice;
    return voice;
}

short SsUtSetVVol(short voice, short left, short right) {
    VoiceCall *call = &s_volume[s_volumeCount++];
    call->voice = voice;
    call->left = left;
    call->right = right;
    return voice;
}

short SsUtChangePitch(short voice, short vabId, short program, short oldNote,
                      short oldFine, short newNote, short newFine) {
    VoiceCall *call = &s_pitch[s_pitchCount++];
    (void)vabId;
    (void)oldNote;
    (void)oldFine;
    call->voice = voice;
    call->program = program;
    call->note = newNote;
    call->fine = newFine;
    return voice;
}

#define CHECK(condition) do {                                                   \
    if (!(condition)) {                                                         \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition); \
        return 1;                                                               \
    }                                                                           \
} while (0)

static void ResetCalls(void) {
    memset(s_keyOn, 0, sizeof(s_keyOn));
    memset(s_keyOff, 0, sizeof(s_keyOff));
    memset(s_volume, 0, sizeof(s_volume));
    memset(s_pitch, 0, sizeof(s_pitch));
    s_keyOnCount = 0;
    s_keyOffCount = 0;
    s_volumeCount = 0;
    s_pitchCount = 0;
}

static int TestPanVoice(void) {
    g_StereoOutput = 1;
    SetPanVoiceTargetVolume(-4, 200);
    CHECK(g_PanVoiceVolumeL == 0 && g_PanVoiceVolumeR == 128);

    g_StereoOutput = 0;
    SetPanVoiceTargetVolume(20, 100);
    CHECK(g_PanVoiceVolumeL == 60 && g_PanVoiceVolumeR == 60);

    ResetCalls();
    g_SoundScale.scale = 64;
    g_SoundScale.vabIds[0] = 7;
    g_PanVoiceActive = 0;
    ApplyPanVoiceVolume();
    CHECK(s_keyOnCount == 1 && s_keyOn[0].voice == 21);
    CHECK(s_keyOn[0].program == 15 && s_volumeCount == 1);
    CHECK(s_volume[0].left == 30 && s_volume[0].right == 30);

    ApplyPanVoiceVolume();
    CHECK(s_keyOnCount == 1 && s_volumeCount == 2);
    g_PanVoiceVolumeL = 1;
    g_PanVoiceVolumeR = 0;
    ApplyPanVoiceVolume();
    CHECK(s_keyOffCount == 1 && s_keyOff[0].voice == 21);
    CHECK(g_PanVoiceActive == 0);

    ResetCalls();
    ForcePanVoiceEnabled(1);
    CHECK(s_keyOnCount == 0 && s_volumeCount == 0);
    g_PanVoiceActive = 1;
    g_PanVoiceVolumeL = 20;
    g_PanVoiceVolumeR = 40;
    ForcePanVoiceEnabled(1);
    CHECK(s_keyOnCount == 1 && s_volumeCount == 1);
    CHECK(s_volume[0].left == 10 && s_volume[0].right == 20);
    ForcePanVoiceEnabled(0);
    CHECK(s_keyOffCount == 1 && s_keyOff[0].voice == 21);
    return 0;
}

static int TestIndexedEffectVoice(void) {
    g_IndexedEffects[0].tone = 30;
    g_IndexedEffects[0].volume = 64;
    g_IndexedEffects[1].tone = 31;
    g_IndexedEffects[1].volume = 96;
    g_IndexedEffects[2].tone = 32;
    g_IndexedEffects[2].volume = 128;
    g_SoundScale.scale = 64;
    g_SoundScale.vabIds[0] = 9;
    g_IndexedEffectIndexPrev = -1;

    SetIndexedEffectVoice(9, 0x2345, 200);
    CHECK(g_IndexedEffectIndex == 2);
    CHECK(g_IndexedEffectPitch == 0x2345 && g_IndexedEffectVolume == 127);
    ResetCalls();
    UpdateIndexedEffectVoice();
    CHECK(s_keyOnCount == 1 && s_keyOn[0].voice == 20);
    CHECK(s_keyOn[0].program == 32);
    CHECK(s_volumeCount == 1 && s_volume[0].left == 63);
    CHECK(s_pitchCount == 1 && s_pitch[0].note == (0x2345 >> 7));
    CHECK(s_pitch[0].fine == (0x2345 & 0x7f));

    g_IndexedEffects[2].volume = INT_MAX;
    g_SoundScale.scale = INT_MAX;
    SetIndexedEffectVoice(2, 0x2345, INT_MAX);
    ResetCalls();
    UpdateIndexedEffectVoice();
    CHECK(s_volumeCount == 1 && s_volume[0].left == 127);
    g_IndexedEffects[2].volume = 128;
    g_SoundScale.scale = 64;

    SetIndexedEffectVoice(1, 0x3456, 80);
    ResetCalls();
    UpdateIndexedEffectVoice();
    CHECK(s_keyOnCount == 1 && s_keyOn[0].program == 31);
    CHECK(s_volume[0].left == 30);

    SetIndexedEffectVoice(-2, 0, 0);
    ResetCalls();
    UpdateIndexedEffectVoice();
    CHECK(s_keyOffCount == 1 && s_keyOff[0].voice == 20);
    CHECK(s_volumeCount == 0 && s_pitchCount == 0);

    ResetCalls();
    ForceIndexedEffectVoiceEnabled(1);
    CHECK(s_keyOnCount == 0 && s_volumeCount == 0);
    g_IndexedEffectIndexPrev = 1;
    ForceIndexedEffectVoiceEnabled(1);
    CHECK(s_keyOnCount == 1 && s_keyOn[0].program == 31);
    CHECK(s_volumeCount == 1 && s_pitchCount == 1);
    ResetCalls();
    ForceIndexedEffectVoiceEnabled(0);
    CHECK(s_keyOffCount == 1 && s_volumeCount == 0 && s_pitchCount == 0);

    g_IndexedEffectIndexPrev = 99;
    ResetCalls();
    ForceIndexedEffectVoiceEnabled(1);
    CHECK(s_keyOnCount == 0 && s_volumeCount == 0 && s_pitchCount == 0);

    g_IndexedEffectIndexPrev = 1;
    g_IndexedEffectIndex = 99;
    ResetCalls();
    UpdateIndexedEffectVoice();
    CHECK(g_IndexedEffectIndex == -1 && g_IndexedEffectIndexPrev == -1);
    CHECK(s_keyOffCount == 1 && s_volumeCount == 0 && s_pitchCount == 0);
    return 0;
}

static int TestBasicEffectVoices(void) {
    memset(g_MusicChannels, 0, sizeof(g_MusicChannels));
    g_SoundScale.scale = 64;
    g_SoundScale.vabIds[0] = 11;
    g_MusicChannels[0].left.value = 12;
    g_MusicChannels[0].right.value = 3;
    g_MusicChannels[0].volLeft = 80;
    g_MusicChannels[0].volRight = 40;
    g_MusicChannels[0].mode = MUSIC_CHANNEL_START;
    g_MusicChannels[1].mode = MUSIC_CHANNEL_STOP;

    ResetCalls();
    UpdateBasicEffectVoices();
    CHECK(s_keyOnCount == 1 && s_keyOn[0].voice == 8);
    CHECK(s_keyOn[0].program == 12 && s_keyOn[0].tone == 3);
    CHECK(s_volumeCount == 1 && s_volume[0].left == 40);
    CHECK(s_volume[0].right == 20);
    CHECK(s_keyOffCount == 1 && s_keyOff[0].voice == 9);
    CHECK(g_MusicChannels[0].mode == MUSIC_CHANNEL_IDLE);
    CHECK(g_MusicChannels[1].mode == MUSIC_CHANNEL_IDLE);

    g_MusicChannels[0].mode = MUSIC_CHANNEL_UPDATE;
    g_MusicChannels[1].mode = MUSIC_CHANNEL_IDLE;
    ResetCalls();
    UpdateBasicEffectVoices();
    CHECK(s_keyOnCount == 0 && s_keyOffCount == 0 && s_volumeCount == 1);

    g_MusicChannels[0].mode = (MusicChannelState)99;
    ResetCalls();
    UpdateBasicEffectVoices();
    CHECK(g_MusicChannels[0].mode == MUSIC_CHANNEL_IDLE);
    CHECK(s_keyOnCount == 0 && s_keyOffCount == 0 && s_volumeCount == 0);

    g_MusicChannels[1].left.value = -1;
    ResetCalls();
    ForceBasicEffectVoicesEnabled(1);
    CHECK(s_keyOnCount == 1 && s_volumeCount == 1);
    CHECK(s_keyOn[0].voice == 8 && s_keyOn[0].program == 12);
    CHECK(s_keyOn[0].tone == 3);
    ResetCalls();
    ForceBasicEffectVoicesEnabled(0);
    CHECK(s_keyOffCount == 2 && s_keyOff[0].voice == 8);
    CHECK(s_keyOff[1].voice == 9);
    return 0;
}

int main(void) {
    CHECK(TestPanVoice() == 0);
    CHECK(TestIndexedEffectVoice() == 0);
    CHECK(TestBasicEffectVoices() == 0);
    puts("effect voice runtime preserves pan, indexed, and basic voice behavior");
    return 0;
}
