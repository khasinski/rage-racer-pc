#include "game/audio.h"
#include "game/sound.h"

void ForceSoundSlotVoicePlayback(s32 enabled) {
    s32 i;

    SetSoundSlotVoicesEnabled(enabled);

    if (enabled != 0) {
        for (i = 0; i < 6; i++) {
            if (g_EngineSoundState.slotActive[i] != 0 &&
                g_SoundSlotTone[i][0] != g_SoundSlotTone[i][1]) {
                PlaySoundSlotVoice(i, g_EngineSoundState.bank, 3);
            }
        }

        for (i = 0; i < 6; i++) {
            if (g_EngineSoundState.slotActive[i] != 0) {
                s32 bend = InterpolateAudioParameter(
                    i * 2, g_EngineSoundState.position,
                    g_EngineSoundState.bank);
                s32 volume = InterpolateAudioParameter(
                    i * 2 + 1, g_EngineSoundState.position,
                    g_EngineSoundState.bank);

                volume = volume * g_EngineSoundState.volumeScale / 128;
                SetSoundSlotTone(i, bend, volume, g_EngineSoundState.bank, 3);
            }
        }
    }
}

void ForceAllEffectVoicesEnabled(s32 enabled) {
    ForcePanVoiceEnabled(enabled);
    ForceBasicEffectVoicesEnabled(enabled);
    ForceIndexedEffectVoiceEnabled(enabled);
    ForcePitchEffectVoicesEnabled(enabled);
    ForceSoundSlotVoicePlayback(enabled);
}
