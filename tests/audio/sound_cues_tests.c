#include "common.h"
#include "game/audio.h"
#include "game/audio_internal.h"
#include "game/sound.h"

#include <stdio.h>
#include <string.h>

SoundScale g_SoundScale;
SoundCueParams g_SoundCueParams[30];
SoundCueParams g_SoundCueParams2[70];
s32 g_SpecialVoiceBits[6];
s16 g_SoundSlotTone[ENGINE_SOUND_SLOT_COUNT][2];
s32 g_SoundCueBank;
s32 g_ActiveSpecialCue;
s32 g_LastSpecialCueRequest;
const char g_MsgTooManyVoices[] = "too many voices\n";

typedef struct KeyCall {
    s32 voice;
    s32 vab;
    s32 program;
    s32 tone;
    s32 left;
    s32 right;
} KeyCall;

static KeyCall s_fixed[16];
static KeyCall s_dynamic[16];
static s32 s_fixedCount;
static s32 s_dynamicCount;
static s32 s_keyStatus[6];
static s32 s_volumeVoice;
static s32 s_volumeLeft;
static s32 s_volumeRight;
static s32 s_bendVoice;
static s32 s_bendVab;
static s32 s_bendProgram;
static s32 s_bendValue;

int DiagnosticsEnabled(const char *key) {
    (void)key;
    return 0;
}

long SpuGetKeyStatus(unsigned long voiceBit) {
    s32 index;

    for (index = 0; index < 6; index++) {
        if ((unsigned long)g_SpecialVoiceBits[index] == voiceBit) {
            return s_keyStatus[index];
        }
    }
    return 0;
}

short SsUtKeyOnV(short voice, short vab, short program, short tone,
                 short note, short fine, short left, short right) {
    KeyCall *call = &s_fixed[s_fixedCount++];
    (void)note;
    (void)fine;
    call->voice = voice;
    call->vab = vab;
    call->program = program;
    call->tone = tone;
    call->left = left;
    call->right = right;
    return voice;
}

short SsUtKeyOn(short vab, short program, short tone, short note, short fine,
                short left, short right) {
    KeyCall *call = &s_dynamic[s_dynamicCount++];
    (void)note;
    (void)fine;
    call->voice = 40 + s_dynamicCount;
    call->vab = vab;
    call->program = program;
    call->tone = tone;
    call->left = left;
    call->right = right;
    return (short)call->voice;
}

short SsUtSetVVol(short voice, short left, short right) {
    s_volumeVoice = voice;
    s_volumeLeft = left;
    s_volumeRight = right;
    return voice;
}

long SsUtPitchBend(long voice, long vab, long program, long note, long bend) {
    (void)note;
    s_bendVoice = (s32)voice;
    s_bendVab = (s32)vab;
    s_bendProgram = (s32)program;
    s_bendValue = (s32)bend;
    return voice;
}

#define CHECK(condition) do {                                                   \
    if (!(condition)) {                                                         \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition); \
        return 1;                                                               \
    }                                                                           \
} while (0)

static void Reset(void) {
    s32 index;

    memset(g_SoundCueParams, 0, sizeof(g_SoundCueParams));
    memset(g_SoundCueParams2, 0, sizeof(g_SoundCueParams2));
    memset(s_keyStatus, 0, sizeof(s_keyStatus));
    for (index = 0; index < 6; index++) {
        g_SpecialVoiceBits[index] = 1 << (index + 18);
    }
    for (index = 0; index < 30; index++) {
        g_SoundCueParams[index].volume = 64;
        g_SoundCueParams[index].vab = 1;
        g_SoundCueParams[index].program = 100 + index;
        g_SoundCueParams[index].toneA = 10 + index;
        g_SoundCueParams[index].toneB = 40 + index;
    }
    for (index = 0; index < 70; index++) {
        g_SoundCueParams2[index].volume = 96;
        g_SoundCueParams2[index].vab = 2;
        g_SoundCueParams2[index].program = 200 + index;
        g_SoundCueParams2[index].toneA = 20 + index;
        g_SoundCueParams2[index].toneB = 50 + index;
    }
    memset(&g_SoundScale, 0, sizeof(g_SoundScale));
    g_SoundScale.scale = 64;
    g_SoundScale.vabIds[1] = 7;
    g_SoundScale.vabIds[2] = 8;
    g_ActiveSpecialCue = -1;
    g_LastSpecialCueRequest = -1;
    s_fixedCount = 0;
    s_dynamicCount = 0;
    s_volumeVoice = -1;
    s_bendVoice = -1;
}

static int TestBankOne(void) {
    Reset();
    g_SoundCueBank = 1;
    PlaySoundCue(-4);
    CHECK(s_fixedCount == 2);
    CHECK(s_fixed[0].voice == 18 && s_fixed[1].voice == 19);
    CHECK(s_fixed[0].program == 100 && s_fixed[0].tone == 10);
    CHECK(s_fixed[1].tone == 40);
    CHECK(s_fixed[0].left == 32 && s_fixed[0].right == 32);

    Reset();
    g_SoundCueBank = 1;
    PlaySoundCue(99);
    CHECK(s_fixed[0].program == 129);

    Reset();
    g_SoundCueBank = 1;
    PlaySoundCue(15);
    CHECK(s_fixedCount == 1 && s_fixed[0].voice == 19);
    CHECK(g_ActiveSpecialCue == 15 && g_LastSpecialCueRequest == 15);
    PlaySoundCue(15);
    CHECK(s_fixedCount == 1);

    Reset();
    g_SoundCueBank = 1;
    s_keyStatus[0] = 1;
    s_keyStatus[1] = 1;
    s_keyStatus[2] = 1;
    s_keyStatus[3] = 1;
    s_keyStatus[4] = 1;
    PlaySoundCue(0);
    CHECK(s_fixedCount == 1 && s_fixed[0].voice == 23);
    return 0;
}

static int TestBankTwo(void) {
    Reset();
    g_SoundCueBank = 2;
    PlaySoundCue(20);
    CHECK(s_dynamicCount == 2);
    CHECK(s_dynamic[0].vab == 8 && s_dynamic[0].program == 220);
    CHECK(s_dynamic[0].tone == 40 && s_dynamic[1].tone == 70);
    CHECK(s_dynamic[0].left == 48 && s_dynamic[0].right == 48);

    Reset();
    g_SoundCueBank = 2;
    PlaySoundCue(99);
    CHECK(s_fixedCount == 2 && s_fixed[0].voice == 22);
    CHECK(s_fixed[1].voice == 23 && s_fixed[0].program == 269);

    Reset();
    g_SoundCueBank = 2;
    s_keyStatus[4] = 1;
    PlaySoundCue(26);
    CHECK(s_fixedCount == 0);
    PlaySoundCue(43);
    CHECK(s_fixedCount == 2);

    Reset();
    g_SoundCueBank = 2;
    PlaySoundCue(16);
    CHECK(s_fixedCount == 1 && s_fixed[0].voice == 19);
    CHECK(s_fixed[0].vab == 7 && s_fixed[0].program == 116);
    CHECK(s_fixed[0].tone == 26 && s_fixed[0].left == 32);
    return 0;
}

static int TestInactiveBankAndEngineSlot(void) {
    Reset();
    g_SoundCueBank = 0;
    PlaySoundCue(3);
    CHECK(s_fixedCount == 0 && s_dynamicCount == 0);

    g_SoundScale.scale = 64;
    g_SoundScale.vabIds[3] = 12;
    g_SoundSlotTone[2][1] = 55;
    SetSoundSlotTone(2, 123, 100, 1, 3);
    CHECK(s_volumeVoice == 16 && s_volumeLeft == 50 && s_volumeRight == 50);
    CHECK(s_bendVoice == 16 && s_bendVab == 12);
    CHECK(s_bendProgram == 55 && s_bendValue == 123);

    s_volumeVoice = -1;
    SetSoundSlotTone(-1, 123, 100, 1, 3);
    SetSoundSlotTone(2, 123, 100, ENGINE_SOUND_BANK_COUNT, 3);
    SetSoundSlotTone(2, 123, 100, 1, AUDIO_SLOT_COUNT);
    CHECK(s_volumeVoice == -1);
    return 0;
}

int main(void) {
    CHECK(TestBankOne() == 0);
    CHECK(TestBankTwo() == 0);
    CHECK(TestInactiveBankAndEngineSlot() == 0);
    puts("sound cues preserve bank routing, deduplication, and engine pitch");
    return 0;
}
