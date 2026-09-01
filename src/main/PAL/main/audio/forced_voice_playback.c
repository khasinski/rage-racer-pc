#include "game/audio.h"
#include "game/sound.h"
#include "psyq/snd.h"

void ForcePanVoiceEnabled(s32 enabled) {
    if (enabled != 0) {
        s32 left = g_PanVoiceVolumeL < 2 ? 0 : g_PanVoiceVolumeL;
        s32 right = g_PanVoiceVolumeR < 2 ? 0 : g_PanVoiceVolumeR;

        left = ClampVoiceVolume(left * g_SoundScale.scale / 128);
        right = ClampVoiceVolume(right * g_SoundScale.scale / 128);
        SsUtSetVVol(0x15, left, right);
        SsUtKeyOnV(0x15, g_SoundScale.vabIds[0], 0xF, 0,
                   0x3C, 0, 0, 0);
    } else {
        SsUtKeyOffV(0x15);
    }
}

void ForceBasicEffectVoicesEnabled(s32 enabled) {
    s32 i;
    s32 left;
    s32 right;

    for (i = 0; i < 2; i++) {
        s32 voice = 8 + i;

        if (enabled != 0) {
            SsUtKeyOnV((s16)voice, g_SoundScale.vabIds[0],
                       g_MusicChannels[i].left.half[0], 0, 0x3C, 0, 0, 0);
            left = ClampVoiceVolume(
                g_MusicChannels[i].volLeft * g_SoundScale.scale / 128);
            right = ClampVoiceVolume(
                g_MusicChannels[i].volRight * g_SoundScale.scale / 128);
            SsUtSetVVol((s16)voice, left, right);
        } else {
            SsUtKeyOffV((s16)voice);
        }
    }
}

void ForceIndexedEffectVoiceEnabled(s32 enabled) {
    s32 index;
    s32 volume;

    if (enabled != 0) {
        index = g_IndexedEffectIndexPrev;
        if (index < 0) {
            return;
        }
        StartIndexedEffectVoice(g_IndexedEffects[index].tone);
    } else {
        StopIndexedEffectVoice();
    }

    index = g_IndexedEffectIndexPrev;
    if (index >= 0) {
        volume = g_IndexedEffectVolume * g_IndexedEffects[index].volume /
                 128 * g_SoundScale.scale / 128;
        volume = ClampVoiceVolume(volume);
        SsUtSetVVol(0x14, volume, volume);
        SsUtChangePitch(0x14, 0, (s16)g_IndexedEffects[index].tone, 0x3C, 0,
                        (s16)(g_IndexedEffectPitch >> 7),
                        g_IndexedEffectPitch & 0x7F);
    }
}

void ForcePitchEffectVoicesEnabled(s32 enabled) {
    s32 index;

    for (index = 0; index < 4; index++) {
        EffectVoice *effect = &g_EffectVoices[index];
        s32 voice = 10 + index;

        if (enabled != 0) {
            s32 volume = ClampVoiceVolume(
                effect->volume * g_SoundScale.scale / 128);

            SsUtKeyOnV((s16)voice, g_SoundScale.vabIds[0],
                       effect->note.half.value, (s16)effect->tone,
                       0x3C, 0, 0, 0);
            SsUtSetVVol((s16)voice, volume, volume);
            SsUtChangePitch((s16)voice, 0, effect->note.half.value, 0x3C, 0,
                            (s16)(effect->pitch.value >> 7),
                            effect->pitch.half.fraction & 0x7F);
        } else {
            SsUtKeyOffV((s16)voice);
        }
    }
}

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
