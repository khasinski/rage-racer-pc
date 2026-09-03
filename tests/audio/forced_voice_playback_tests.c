#include "common.h"
#include "game/audio.h"
#include "game/audio_internal.h"

#include <stdio.h>

static s32 s_forceOrder[5];
static s32 s_forceCalls;

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

void ForceSoundSlotVoicePlayback(s32 enabled) {
    s_forceOrder[s_forceCalls++] = 50 + enabled;
}

#define CHECK(condition) do {                                                   \
    if (!(condition)) {                                                         \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition); \
        return 1;                                                               \
    }                                                                           \
} while (0)

int main(void) {
    ForceAllEffectVoicesEnabled(1);
    CHECK(s_forceCalls == 5);
    CHECK(s_forceOrder[0] == 11 && s_forceOrder[1] == 21);
    CHECK(s_forceOrder[2] == 31 && s_forceOrder[3] == 41);
    CHECK(s_forceOrder[4] == 51);

    puts("forced voice playback restores active engine slots and all families");
    return 0;
}
