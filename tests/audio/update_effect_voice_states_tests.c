#include "common.h"
#include "game/audio.h"
#include "game/sound.h"

#include <stdio.h>
#include <string.h>

EffectVoice g_EffectVoices[4];
SoundScale g_SoundScale;

static s32 s_keyOnVoice = -1;
static s32 s_keyOnProgram;
static s32 s_keyOnTone;
static s32 s_keyOffVoice = -1;
static s32 s_volumeCalls;
static s32 s_volumeVoice[2];
static s32 s_volumeValue[2];
static s32 s_pitchCalls;
static s32 s_pitchVoice[2];
static s32 s_pitchProgram[2];
static s32 s_pitchNote[2];
static s32 s_pitchFine[2];

short SsUtKeyOnV(short voice, short vabId, short program, short tone,
                 short note, short fine, short volumeLeft,
                 short volumeRight) {
    (void)vabId; (void)note; (void)fine; (void)volumeLeft; (void)volumeRight;
    s_keyOnVoice = voice;
    s_keyOnProgram = program;
    s_keyOnTone = tone;
    return voice;
}
short SsUtKeyOffV(short voice) {
    s_keyOffVoice = voice;
    return voice;
}
short SsUtSetVVol(short voice, short left, short right) {
    s_volumeVoice[s_volumeCalls] = voice;
    s_volumeValue[s_volumeCalls] = left;
    if (left != right) {
        return -1;
    }
    s_volumeCalls++;
    return 0;
}
short SsUtChangePitch(short voice, short vabId, short program, short oldNote,
                      short oldFine, short newNote, short newFine) {
    (void)vabId; (void)oldNote; (void)oldFine;
    s_pitchVoice[s_pitchCalls] = voice;
    s_pitchProgram[s_pitchCalls] = program;
    s_pitchNote[s_pitchCalls] = newNote;
    s_pitchFine[s_pitchCalls] = newFine;
    s_pitchCalls++;
    return 0;
}

#define CHECK(condition) do {                                                  \
    if (!(condition)) {                                                        \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition);\
        return 1;                                                              \
    }                                                                          \
} while (0)

int main(void) {
    memset(g_EffectVoices, 0, sizeof(g_EffectVoices));
    memset(&g_SoundScale, 0, sizeof(g_SoundScale));
    g_SoundScale.scale = 128;
    g_SoundScale.vabIds[0] = 7;

    g_EffectVoices[0].state = 0;
    g_EffectVoices[0].note.value = 22;
    g_EffectVoices[0].tone = 3;
    g_EffectVoices[0].pitch.value = 0x12345;
    g_EffectVoices[0].volume = 64;
    g_EffectVoices[1].state = 2;
    g_EffectVoices[1].note.value = 33;
    g_EffectVoices[1].pitch.value = 0x23456;
    g_EffectVoices[1].volume = 200;
    g_EffectVoices[2].state = 1;
    g_EffectVoices[3].state = -1;

    UpdateEffectVoiceStates();
    CHECK(s_keyOnVoice == 10 && s_keyOnProgram == 22 && s_keyOnTone == 3);
    CHECK(s_keyOffVoice == 12);
    CHECK(s_volumeCalls == 2 && s_pitchCalls == 2);
    CHECK(s_volumeVoice[0] == 10 && s_volumeValue[0] == 64);
    CHECK(s_volumeVoice[1] == 11 && s_volumeValue[1] == 128);
    CHECK(s_pitchVoice[0] == 10 && s_pitchProgram[0] == 22);
    CHECK(s_pitchNote[0] == (s16)(0x12345 >> 7));
    CHECK(s_pitchFine[0] == (0x12345 & 0x7F));
    CHECK(g_EffectVoices[0].state == -1);
    CHECK(g_EffectVoices[1].state == -1);
    CHECK(g_EffectVoices[2].state == -1);

    puts("effect voice states preserve SPU key, volume, and pitch updates");
    return 0;
}
