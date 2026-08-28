#include "game/audio.h"
#include "game/sound.h"

void SetVabSlotVoiceEnabled(s32 voice, s32 enabled, s32 vabSlot) {
    s32 *state;

    if (enabled != 0) {
        s32 *base = g_EngineSoundState.slotActive;

        state = base + voice;
        if (*state == 0) {
            StartVabSlotVoice(voice, 0, (s16)vabSlot);
            *state = 1;
        }
    } else {
        s32 *base = g_EngineSoundState.slotActive;

        state = base + voice;
        if (*state != 0) {
            StopDirectVoice(voice);
            *state = 0;
        }
    }
}

void SetSequenceVolume(s32 volume) {
    g_SeqVolume = volume;
    SsSeqSetVol(g_SeqHandle.value, (s16)volume, (s16)volume);
}
