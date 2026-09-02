#include "game/diagnostics.h"
#include "game/audio.h"
#include "psyq/snd.h"
#include "game/sound.h"

#include <stdio.h>

enum {
    CUE_VOLUME_FULL = 128,
    SOUND_BASE_NOTE = 0x3C,
    BANK_ONE_CUE_COUNT = 30,
    BANK_TWO_CUE_COUNT = 70,
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

static void StartSoundCueVoice(s32 cue, s32 volL, s32 volR) {
    const SoundCueParams *params = g_SoundCueBank == 1
                                       ? &g_SoundCueParams[cue]
                                       : &g_SoundCueParams2[cue];
    s32 busy[POOLED_VOICE_COUNT];
    s32 result = -1;
    s32 i;

    volL = ScaleCueVolume(volL, params->volume);
    volR = ScaleCueVolume(volR, params->volume);

    if (g_SoundCueBank == 1) {
        for (i = 0; i < POOLED_VOICE_COUNT; i++) {
            busy[i] = SpuGetKeyStatus(g_SpecialVoiceBits[i]);
        }
        for (i = 0; i < POOLED_VOICE_COUNT; i++) {
            if (busy[i] == 0) {
                result = (s16)SsUtKeyOnV(
                    (s16)(i + POOLED_VOICE_FIRST),
                    g_SoundScale.vabIds[params->vab],
                    (s16)params->program, (s16)params->toneA,
                    SOUND_BASE_NOTE, 0, (s16)volL, (s16)volR);
                g_SpecialCueVoiceA = result;
                busy[i] = 1;
                break;
            }
        }
        for (i = 0; i < POOLED_VOICE_COUNT; i++) {
            if (busy[i] == 0) {
                result = (s16)SsUtKeyOnV(
                    (s16)(i + POOLED_VOICE_FIRST),
                    g_SoundScale.vabIds[params->vab],
                    (s16)params->program, (s16)params->toneB,
                    SOUND_BASE_NOTE, 0, (s16)volL, (s16)volR);
                g_SpecialCueVoiceB = result;
                break;
            }
        }
    } else {
        result = (s16)SsUtKeyOn(g_SoundScale.vabIds[params->vab],
                                params->program, params->toneA,
                                SOUND_BASE_NOTE, 0, volL, volR);
        g_SpecialCueVoiceA = result;
        result = (s16)SsUtKeyOn(g_SoundScale.vabIds[params->vab],
                                params->program, params->toneB,
                                SOUND_BASE_NOTE, 0, volL, volR);
        g_SpecialCueVoiceB = result;
    }

    if (result < 0) {
        printf("%s", g_MsgTooManyVoices);
    }
}


static void StartSingleSpecialCue(s32 cue, s32 volume) {
    s32 result = -1;
    s32 voiceVolume;
    s32 vab;
    s32 program;
    s32 tone;

    g_SpecialCueVoiceA = -1;
    g_SpecialCueVoiceB = -1;

    if (g_ActiveSpecialCue != cue) {
        vab = g_SoundCueParams[cue].vab;
        program = g_SoundCueParams[cue].program;
        tone = g_SoundCueParams[cue].toneA;
        voiceVolume = ScaleCueVolume(volume, g_SoundCueParams[cue].volume);
        result = (s16)SsUtKeyOnV(SINGLE_SPECIAL_VOICE,
                                 g_SoundScale.vabIds[vab],
                                 (s16)program, (s16)tone, SOUND_BASE_NOTE, 0,
                                 voiceVolume, voiceVolume);
        g_SpecialCueVoiceA = result;
    }

    g_ActiveSpecialCue = cue;
}

static void StartSpecialCueVoice(s32 cue, s32 volumeLeft, s32 volumeRight) {
    s32 vab;
    s32 prog;
    s32 tone;
    s32 result = -1;
    s32 baseVolume = g_SoundCueParams2[cue].volume;

    vab = g_SoundCueParams2[cue].vab;
    prog = g_SoundCueParams2[cue].program;
    tone = g_SoundCueParams2[cue].toneA;
    volumeLeft = ScaleCueVolume(volumeLeft, baseVolume);
    volumeRight = ScaleCueVolume(volumeRight, baseVolume);

    if (SpuGetKeyStatus(g_SpecialVoiceBits[STEREO_SPECIAL_STATUS_INDEX]) == 0 ||
        cue == ALWAYS_RESTART_SPECIAL_CUE_A ||
        cue == ALWAYS_RESTART_SPECIAL_CUE_B) {
        result = (s16)SsUtKeyOnV(STEREO_SPECIAL_VOICE_LEFT,
                                 g_SoundScale.vabIds[vab],
                                 (s16)prog, (s16)tone, SOUND_BASE_NOTE, 0,
                                 (s16)volumeLeft, (s16)volumeRight);
        g_SpecialCueVoiceA = result;
        result = (s16)SsUtKeyOnV(STEREO_SPECIAL_VOICE_RIGHT,
                                 g_SoundScale.vabIds[vab],
                                 (s16)prog, (s16)(tone + 1), SOUND_BASE_NOTE, 0,
                                 (s16)volumeLeft, (s16)volumeRight);
        g_SpecialCueVoiceB = result;
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
        cue = ClampCueIndex(cue, BANK_ONE_CUE_COUNT);
        if (IsRepeatedSpecialCue(cue)) {
            if (cue != g_LastSpecialCueRequest) {
                g_LastSpecialCueRequest = cue;
                StartSingleSpecialCue(cue, CUE_VOLUME_FULL);
            }
            return;
        }
        StartSoundCueVoice(cue, CUE_VOLUME_FULL, CUE_VOLUME_FULL);
        return;
    }

    if (g_SoundCueBank == 2) {
        cue = ClampCueIndex(cue, BANK_TWO_CUE_COUNT);
        if (IsRepeatedSpecialCue(cue)) {
            if (cue != g_LastSpecialCueRequest) {
                g_LastSpecialCueRequest = cue;
                StartSingleSpecialCue(cue, CUE_VOLUME_FULL);
            }
            return;
        }
        if (cue < BANK_TWO_FIXED_VOICE_FIRST_CUE) {
            StartSoundCueVoice(cue, CUE_VOLUME_FULL, CUE_VOLUME_FULL);
            return;
        }
        StartSpecialCueVoice(cue, CUE_VOLUME_FULL, CUE_VOLUME_FULL);
    }
}


/* Sets one engine-sound slot: scales `volume` by the global effect scale,
 * pushes it to the slot's voice, then re-pitches that voice to the tone at
 * g_SoundSlotTone[slot][toneIndex]. */
void SetSoundSlotTone(s32 slot, s32 bend, s32 volume, s32 toneIndex, u16 vabSlot) {
    s16 voice = (s16)(slot + ENGINE_SLOT_VOICE_FIRST);
    s32 scaledVolume = ClampVoiceVolume(
        volume * g_SoundScale.scale / 128);

    SsUtSetVVol(voice, scaledVolume, scaledVolume);
    SsUtPitchBend(voice, g_SoundScale.vabIds[(s16)vabSlot],
                  g_SoundSlotTone[slot][toneIndex], SOUND_BASE_NOTE,
                  (s16)bend);
}
