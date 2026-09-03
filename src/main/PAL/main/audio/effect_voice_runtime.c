#include "game/audio.h"
#include "game/audio_internal.h"
#include "psyq/snd.h"
#include "game/sound.h"

enum {
    BASIC_EFFECT_VOICE_FIRST = 8,
    BASIC_EFFECT_VOICE_COUNT = 2,
    INDEXED_EFFECT_VOICE = 20,
    PAN_EFFECT_VOICE = 21,
    INDEXED_EFFECT_COUNT = 3,
    PAN_EFFECT_PROGRAM = 15,
    EFFECT_BASE_NOTE = 0x3C,
    AUDIBLE_PAN_VOLUME_MIN = 2,
    SOUND_SCALE_ONE = 128,
};

static s32 ScaleVoiceVolume(s32 volume) {
    return ClampVoiceVolume(volume * g_SoundScale.scale / SOUND_SCALE_ONE);
}

static int GetPanVoiceOutputVolume(s32 *left, s32 *right) {
    int audible;

    *left = g_PanVoiceVolumeL < AUDIBLE_PAN_VOLUME_MIN ? 0
                                                       : g_PanVoiceVolumeL;
    *right = g_PanVoiceVolumeR < AUDIBLE_PAN_VOLUME_MIN ? 0
                                                        : g_PanVoiceVolumeR;
    audible = *left != 0 || *right != 0;
    *left = ScaleVoiceVolume(*left);
    *right = ScaleVoiceVolume(*right);
    return audible;
}

void SetPanVoiceTargetVolume(s32 left, s32 right) {
    left = ClampVoiceVolume(left);
    right = ClampVoiceVolume(right);

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
    s32 audible;

    audible = GetPanVoiceOutputVolume(&left, &right);

    if (audible != 0) {
        SsUtSetVVol(PAN_EFFECT_VOICE, left, right);
        if (g_PanVoiceActive == 0) {
            SsUtKeyOnV(PAN_EFFECT_VOICE, g_SoundScale.vabIds[0],
                       PAN_EFFECT_PROGRAM, 0, EFFECT_BASE_NOTE, 0, 0, 0);
        }
    } else if (g_PanVoiceActive != 0) {
        SsUtKeyOffV(PAN_EFFECT_VOICE);
    }

    g_PanVoiceActive = audible;
}

void ForcePanVoiceEnabled(s32 enabled) {
    if (enabled != 0) {
        s32 left;
        s32 right;

        if (g_PanVoiceActive == 0) {
            return;
        }
        GetPanVoiceOutputVolume(&left, &right);
        SsUtSetVVol(PAN_EFFECT_VOICE, left, right);
        SsUtKeyOnV(PAN_EFFECT_VOICE, g_SoundScale.vabIds[0],
                   PAN_EFFECT_PROGRAM, 0, EFFECT_BASE_NOTE, 0, 0, 0);
    } else {
        SsUtKeyOffV(PAN_EFFECT_VOICE);
    }
}

static void StartIndexedEffectVoice(s32 baseTone) {
    SsUtKeyOnV(INDEXED_EFFECT_VOICE, g_SoundScale.vabIds[0], (s16)baseTone,
               0, EFFECT_BASE_NOTE, 0, 0, 0);
}

static void StopIndexedEffectVoice(void) {
    SsUtKeyOffV(INDEXED_EFFECT_VOICE);
}

static void ApplyIndexedEffectVoiceOutput(s32 index) {
    s32 volume = g_IndexedEffectVolume * g_IndexedEffects[index].volume /
                 SOUND_SCALE_ONE;

    volume = ScaleVoiceVolume(volume);
    SsUtSetVVol(INDEXED_EFFECT_VOICE, volume, volume);
    SsUtChangePitch(INDEXED_EFFECT_VOICE, 0,
                    (s16)g_IndexedEffects[index].tone, EFFECT_BASE_NOTE, 0,
                    (s16)(g_IndexedEffectPitch >> 7),
                    g_IndexedEffectPitch & 0x7F);
}

void SetIndexedEffectVoice(s32 index, s32 phase, s32 volume) {
    if (index < -1) {
        index = -1;
    } else if (index >= INDEXED_EFFECT_COUNT) {
        index = INDEXED_EFFECT_COUNT - 1;
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
        ApplyIndexedEffectVoiceOutput(index);
    }

    g_IndexedEffectIndexPrev = g_IndexedEffectIndex;
}

void ForceIndexedEffectVoiceEnabled(s32 enabled) {
    s32 index = g_IndexedEffectIndexPrev;

    if (enabled != 0) {
        if (index < 0) {
            return;
        }
        StartIndexedEffectVoice(g_IndexedEffects[index].tone);
        ApplyIndexedEffectVoiceOutput(index);
    } else {
        StopIndexedEffectVoice();
    }
}

void UpdateBasicEffectVoices(void) {
    s32 i;

    for (i = 0; i < BASIC_EFFECT_VOICE_COUNT; i++) {
        MusicChannel *channel = &g_MusicChannels[i];
        s16 voice = (s16)(BASIC_EFFECT_VOICE_FIRST + i);

        switch (channel->mode) {
        case MUSIC_CHANNEL_START:
            SsUtKeyOnV(voice, g_SoundScale.vabIds[0], channel->left.half[0],
                       channel->right.half[0], EFFECT_BASE_NOTE, 0, 0, 0);
            /* A newly keyed voice needs the same volume update. */
            RAGE_FALLTHROUGH;
        case MUSIC_CHANNEL_UPDATE:
            SsUtSetVVol(
                voice,
                ScaleVoiceVolume(channel->volLeft),
                ScaleVoiceVolume(channel->volRight));
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

void ForceBasicEffectVoicesEnabled(s32 enabled) {
    s32 index;

    for (index = 0; index < BASIC_EFFECT_VOICE_COUNT; index++) {
        MusicChannel *channel = &g_MusicChannels[index];
        s16 voice = (s16)(BASIC_EFFECT_VOICE_FIRST + index);

        if (enabled != 0) {
            s32 left = ScaleVoiceVolume(channel->volLeft);
            s32 right = ScaleVoiceVolume(channel->volRight);

            if (channel->left.value < 0) {
                continue;
            }
            SsUtKeyOnV(voice, g_SoundScale.vabIds[0], channel->left.half[0],
                       0, EFFECT_BASE_NOTE, 0, 0, 0);
            SsUtSetVVol(voice, left, right);
        } else {
            SsUtKeyOffV(voice);
        }
    }
}
