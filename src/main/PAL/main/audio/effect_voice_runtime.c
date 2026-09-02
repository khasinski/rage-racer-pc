#include "game/audio.h"
#include "psyq/snd.h"
#include "game/sound.h"

void SetPanVoiceTargetVolume(s32 left, s32 right) {
    if (left >= 0) {
        if (left > 0x80) {
            left = 0x80;
        }
    } else {
        left = 0;
    }

    if (right >= 0) {
        if (right > 0x80) {
            right = 0x80;
        }
    } else {
        right = 0;
    }

    if (g_StereoOutput != 0) {
        g_PanVoiceVolumeL = left;
        g_PanVoiceVolumeR = right;
    } else {
        s32 temp = (left + right) / 2;

        g_PanVoiceVolumeL = temp;
        g_PanVoiceVolumeR = temp;
    }
}

void ApplyPanVoiceVolume(void) {
    s32 left;
    s32 right;
    s32 changed;

    left = g_PanVoiceVolumeL < 2 ? 0 : g_PanVoiceVolumeL;
    right = g_PanVoiceVolumeR < 2 ? 0 : g_PanVoiceVolumeR;
    changed = left != 0 || right != 0;

    if (changed != 0) {
        left = ClampVoiceVolume(left * g_SoundScale.scale / 128);
        right = ClampVoiceVolume(right * g_SoundScale.scale / 128);

        SsUtSetVVol(0x15, left, right);
        if (g_PanVoiceActive == 0) {
            SsUtKeyOnV(0x15, g_SoundScale.vabIds[0], 0xF, 0,
                       0x3C, 0, 0, 0);
        }
    } else if (g_PanVoiceActive != 0) {
        SsUtKeyOffV(0x15);
    }

    g_PanVoiceActive = changed;
}

void StartIndexedEffectVoice(s32 baseTone) {
    SsUtKeyOnV(0x14, g_SoundScale.vabIds[0], (s16)baseTone, 0, 0x3C, 0, 0, 0);
}

void StopIndexedEffectVoice(void) {
    SsUtKeyOffV(0x14);
}

void SetIndexedEffectVoice(s32 index, s32 phase, s32 volume) {
    if (index >= -1) {
        if (index >= 3) {
            index = 2;
        }
    } else {
        index = -1;
    }

    volume = ClampCueLevel(volume);

    g_IndexedEffectIndex = index;
    if (index >= 0) {
        g_IndexedEffectVolume = volume;
        g_IndexedEffectPitch = phase;
    }
}

void UpdateIndexedEffectVoice(void) {
    s32 index;
    s32 previous;

    /* Start on the way in, stop on the way out, restart on a change. The
     * early exit retail had for "nothing playing and nothing asked for" is
     * the same condition the rest of the function is already guarded by. */
    previous = g_IndexedEffectIndexPrev;
    index = g_IndexedEffectIndex;
    if (previous < 0) {
        if (index >= 0) {
            StartIndexedEffectVoice(g_IndexedEffects[index].tone);
        }
    } else if (index < 0) {
        StopIndexedEffectVoice();
    } else if (index != previous) {
        StartIndexedEffectVoice(g_IndexedEffects[index].tone);
    }

    if (index >= 0) {
        s32 volume = g_IndexedEffectVolume * g_IndexedEffects[index].volume /
                     128 * g_SoundScale.scale / 128;

        volume = ClampVoiceVolume(volume);
        SsUtSetVVol(0x14, volume, volume);
        SsUtChangePitch(0x14, 0, (s16)g_IndexedEffects[index].tone, 0x3C, 0,
                        (s16)(g_IndexedEffectPitch >> 7),
                        g_IndexedEffectPitch & 0x7F);
    }

    g_IndexedEffectIndexPrev = g_IndexedEffectIndex;
}

void UpdateBasicEffectVoices(void) {
    s32 i;

    for (i = 0; i < 2; i++) {
        MusicChannel *channel = &g_MusicChannels[i];
        s16 voice = (s16)(8 + i);

        switch (channel->mode) {
        case MUSIC_CHANNEL_START:
            SsUtKeyOnV(voice, g_SoundScale.vabIds[0], channel->left.half[0],
                       channel->right.half[0], 0x3C, 0, 0, 0);
            /* A newly keyed voice needs the same volume update. */
            RAGE_FALLTHROUGH;
        case MUSIC_CHANNEL_UPDATE:
            SsUtSetVVol(
                voice,
                ClampVoiceVolume(channel->volLeft * g_SoundScale.scale / 128),
                ClampVoiceVolume(channel->volRight * g_SoundScale.scale / 128));
            channel->mode = MUSIC_CHANNEL_IDLE;
            break;
        case MUSIC_CHANNEL_STOP:
            SsUtKeyOffV(voice);
            channel->mode = MUSIC_CHANNEL_IDLE;
            break;
        case MUSIC_CHANNEL_IDLE:
            break;
        }
    }
}
