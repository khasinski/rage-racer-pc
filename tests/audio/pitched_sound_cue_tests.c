#include "common.h"
#include "game/audio.h"
#include "game/sound.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

EffectCueBank g_EffectCueTable[EFFECT_CUE_BANK_COUNT];
EffectVoice g_EffectVoices[AUDIO_EFFECT_VOICE_COUNT];

#define CHECK(condition) do {                                                  \
    if (!(condition)) {                                                        \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition);\
        return 1;                                                              \
    }                                                                          \
} while (0)

static void Reset(void) {
    s32 bank;
    s32 voice;

    memset(g_EffectCueTable, 0, sizeof(g_EffectCueTable));
    memset(g_EffectVoices, 0, sizeof(g_EffectVoices));
    for (bank = 0; bank < EFFECT_CUE_BANK_COUNT; bank++) {
        g_EffectCueTable[bank].voiceCount = 2;
        g_EffectCueTable[bank].volumeScale = 64 + bank * 16;
        for (voice = 0; voice < 2; voice++) {
            g_EffectCueTable[bank].programs[voice].note =
                20 + bank * 10 + voice;
            g_EffectCueTable[bank].programs[voice].tone = bank + voice;
        }
    }
}

int main(void) {
    Reset();
    SetPitchedSoundCue(0, 0x2345, 64);
    CHECK(g_EffectVoices[0].state == EFFECT_VOICE_START &&
          g_EffectVoices[1].state == EFFECT_VOICE_START);
    CHECK(g_EffectVoices[0].note.value == 20);
    CHECK(g_EffectVoices[1].note.value == 21);
    CHECK(g_EffectVoices[0].pitch.value == 0x2345);
    CHECK(g_EffectVoices[0].volume == 32);

    SetPitchedSoundCue(0, 0x3456, 200);
    CHECK(g_EffectVoices[0].state == EFFECT_VOICE_UPDATE &&
          g_EffectVoices[1].state == EFFECT_VOICE_UPDATE);
    CHECK(g_EffectVoices[0].volume == 63);

    Reset();
    g_EffectVoices[2].note.value = 99;
    g_EffectVoices[3].note.value = 100;
    SetPitchedSoundCue(1, 0, 0);
    CHECK(g_EffectVoices[2].note.value == 99);
    g_EffectVoices[2].note.value = 30;
    g_EffectVoices[3].note.value = 31;
    SetPitchedSoundCue(1, 0, 0);
    CHECK(g_EffectVoices[2].state == EFFECT_VOICE_STOP &&
          g_EffectVoices[3].state == EFFECT_VOICE_STOP);
    CHECK(g_EffectVoices[2].note.value == -1);
    CHECK(g_EffectVoices[2].pitch.value == 0x1E00);

    Reset();
    SetPitchedSoundCue(5, 0x1111, 64);
    CHECK(g_EffectVoices[2].note.value == 40);
    CHECK(g_EffectVoices[3].note.value == 41);
    SetPitchedSoundCue(-2, 0x2222, 64);
    CHECK(g_EffectVoices[0].note.value == 20);

    Reset();
    g_EffectCueTable[0].voiceCount = 99;
    g_EffectVoices[2].note.value = 1234;
    SetPitchedSoundCue(0, 0x3333, 64);
    CHECK(g_EffectVoices[0].note.value == 20 &&
          g_EffectVoices[1].note.value == 21);
    CHECK(g_EffectVoices[2].note.value == 1234);

    Reset();
    g_EffectCueTable[0].volumeScale = INT_MAX;
    SetPitchedSoundCue(0, 0, INT_MAX);
    CHECK(g_EffectVoices[0].volume == 127);

    puts("pitched sound cues preserve bank routing, reuse, and reset");
    return 0;
}
