#include "game/diagnostics.h"
#include "game/audio.h"
#include "game/audio_internal.h"
#include "psyq/snd.h"
#include "game/sound.h"

#include <stdio.h>

enum {
    CUE_VOLUME_FULL = 128,
    SOUND_BASE_NOTE = 0x3C,
    REPEATED_SPECIAL_CUE_FIRST = 15,
    REPEATED_SPECIAL_CUE_LAST = 17,
    BANK_TWO_FIXED_VOICE_FIRST_CUE = 25,
    POOLED_VOICE_FIRST = 18,
    POOLED_VOICE_COUNT = 6,
    SINGLE_SPECIAL_VOICE = 19,
    STEREO_SPECIAL_VOICE_LEFT = 22,
    STEREO_SPECIAL_VOICE_RIGHT = 23,
    STEREO_SPECIAL_STATUS_INDEX = 4,
    ALWAYS_RESTART_SPECIAL_CUE_A = 0x2B,
    ALWAYS_RESTART_SPECIAL_CUE_B = 0x3D,
    ENGINE_SLOT_VOICE_FIRST = 14,
};

static s32 ScaleCueVolume(s32 volume, s32 cueScale) {
    return volume * cueScale / 128 * g_SoundScale.scale / 128;
}

static s32 StartPooledCueTone(const SoundCueParams *params, s32 tone,
                              s32 volumeLeft, s32 volumeRight,
                              s32 busy[POOLED_VOICE_COUNT]) {
    s32 i;

    for (i = 0; i < POOLED_VOICE_COUNT; i++) {
        if (busy[i] == 0) {
            s32 voice = SsUtKeyOnV(
                (s16)(i + POOLED_VOICE_FIRST),
                g_SoundScale.vabIds[params->vab],
                (s16)params->program, (s16)tone, SOUND_BASE_NOTE, 0,
                (s16)volumeLeft, (s16)volumeRight);

            busy[i] = 1;
            return voice;
        }
    }
    return -1;
}

static void StartPairedSoundCue(s32 cue, s32 volL, s32 volR) {
    const SoundCueParams *params = g_SoundCueBank == 1
                                       ? &g_SoundCueParams[cue]
                                       : &g_SoundCueParams2[cue];
    s32 busy[POOLED_VOICE_COUNT];
    int failed;
    s32 voiceA;
    s32 voiceB;
    s32 i;

    volL = ScaleCueVolume(volL, params->volume);
    volR = ScaleCueVolume(volR, params->volume);

    if (g_SoundCueBank == 1) {
        for (i = 0; i < POOLED_VOICE_COUNT; i++) {
            busy[i] = SpuGetKeyStatus(g_SpecialVoiceBits[i]);
        }
        voiceA = StartPooledCueTone(
            params, params->toneA, volL, volR, busy);
        voiceB = StartPooledCueTone(
            params, params->toneB, volL, volR, busy);
    } else {
        voiceA = SsUtKeyOn(
            g_SoundScale.vabIds[params->vab], params->program,
            params->toneA, SOUND_BASE_NOTE, 0, volL, volR);
        voiceB = SsUtKeyOn(
            g_SoundScale.vabIds[params->vab], params->program,
            params->toneB, SOUND_BASE_NOTE, 0, volL, volR);
    }

    failed = voiceA < 0 || voiceB < 0;

    if (failed) {
        printf("%s", g_MsgTooManyVoices);
    }
}

/* Cues 15..17 always live in the main cue bank, including while the race cue
 * bank is active. They share fixed voice 19 and suppress repeat requests. */
static void StartSharedSingleCue(s32 cue, s32 volume) {
    const SoundCueParams *params = &g_SoundCueParams[cue];
    s32 voiceVolume;

    if (g_ActiveSpecialCue != cue) {
        voiceVolume = ScaleCueVolume(volume, params->volume);
        SsUtKeyOnV(
            SINGLE_SPECIAL_VOICE, g_SoundScale.vabIds[params->vab],
            (s16)params->program, (s16)params->toneA, SOUND_BASE_NOTE, 0,
            voiceVolume, voiceVolume);
    }

    g_ActiveSpecialCue = cue;
}

static void StartSpecialCueVoice(s32 cue, s32 volumeLeft, s32 volumeRight) {
    const SoundCueParams *params = &g_SoundCueParams2[cue];

    volumeLeft = ScaleCueVolume(volumeLeft, params->volume);
    volumeRight = ScaleCueVolume(volumeRight, params->volume);

    if (SpuGetKeyStatus(g_SpecialVoiceBits[STEREO_SPECIAL_STATUS_INDEX]) == 0 ||
        cue == ALWAYS_RESTART_SPECIAL_CUE_A ||
        cue == ALWAYS_RESTART_SPECIAL_CUE_B) {
        SsUtKeyOnV(
            STEREO_SPECIAL_VOICE_LEFT, g_SoundScale.vabIds[params->vab],
            (s16)params->program, (s16)params->toneA, SOUND_BASE_NOTE, 0,
            (s16)volumeLeft, (s16)volumeRight);
        SsUtKeyOnV(
            STEREO_SPECIAL_VOICE_RIGHT, g_SoundScale.vabIds[params->vab],
            (s16)params->program, (s16)(params->toneA + 1), SOUND_BASE_NOTE, 0,
            (s16)volumeLeft, (s16)volumeRight);
    }
}

static s32 ClampCueIndex(s32 cue, s32 cueCount) {
    if (cue < 0) {
        return 0;
    }
    return cue >= cueCount ? cueCount - 1 : cue;
}

static int IsRepeatedSpecialCue(s32 cue) {
    return cue >= REPEATED_SPECIAL_CUE_FIRST &&
           cue <= REPEATED_SPECIAL_CUE_LAST;
}

void PlaySoundCue(s32 cue) {
    if (DiagnosticsEnabled("sound_cue_trace")) {
        fprintf(stderr, "rage-port: sound cue=0x%02x\n", (unsigned)cue);
    }

    if (g_SoundCueBank == 1) {
        cue = ClampCueIndex(cue, MAIN_SOUND_CUE_COUNT);
        if (IsRepeatedSpecialCue(cue)) {
            if (cue != g_LastSpecialCueRequest) {
                g_LastSpecialCueRequest = cue;
                StartSharedSingleCue(cue, CUE_VOLUME_FULL);
            }
            return;
        }
        StartPairedSoundCue(cue, CUE_VOLUME_FULL, CUE_VOLUME_FULL);
        return;
    }

    if (g_SoundCueBank == 2) {
        cue = ClampCueIndex(cue, RACE_SOUND_CUE_COUNT);
        if (IsRepeatedSpecialCue(cue)) {
            if (cue != g_LastSpecialCueRequest) {
                g_LastSpecialCueRequest = cue;
                StartSharedSingleCue(cue, CUE_VOLUME_FULL);
            }
            return;
        }
        if (cue < BANK_TWO_FIXED_VOICE_FIRST_CUE) {
            StartPairedSoundCue(cue, CUE_VOLUME_FULL, CUE_VOLUME_FULL);
            return;
        }
        StartSpecialCueVoice(cue, CUE_VOLUME_FULL, CUE_VOLUME_FULL);
    }
}

/* Sets one engine-sound slot: scales `volume` by the global effect scale,
 * pushes it to the slot's voice, then re-pitches that voice to the tone at
 * g_SoundSlotTone[slot][toneIndex]. */
void SetSoundSlotTone(s32 slot, s32 bend, s32 volume, s32 toneIndex, u16 vabSlot) {
    if ((u32)slot >= ENGINE_SOUND_SLOT_COUNT ||
        (u32)toneIndex >= ENGINE_SOUND_BANK_COUNT ||
        vabSlot >= AUDIO_SLOT_COUNT) {
        return;
    }

    s16 voice = (s16)(slot + ENGINE_SLOT_VOICE_FIRST);
    s32 scaledVolume = ClampVoiceVolume(
        volume * g_SoundScale.scale / 128);

    SsUtSetVVol(voice, scaledVolume, scaledVolume);
    SsUtPitchBend(voice, g_SoundScale.vabIds[vabSlot],
                  g_SoundSlotTone[slot][toneIndex], SOUND_BASE_NOTE,
                  (s16)bend);
}
