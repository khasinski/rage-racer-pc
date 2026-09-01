#include "game/diagnostics.h"
#include "game/audio.h"
#include "psyq/snd.h"
#include "game/sound.h"

#include <stdio.h>
#include <stdlib.h>

static s32 ScaleCueVolume(s32 volume, s32 cueScale) {
    return volume * cueScale / 128 * g_SoundScale.scale / 128;
}

static s32 StartSoundCueVoice(s32 cue, s32 volL, s32 volR) {
    const s32 *voiceBits;
    s32 busy[6];
    s32 tone2;
    s32 vab;
    s32 prog;
    s32 tone;
    s32 baseVol;
    s32 result = -1;
    s32 i;

    tone = 0;
    voiceBits = g_SpecialVoiceBits;
    tone2 = 1;
    if (g_SoundCueBank == 1) {
        tone2 = g_SoundCueParams[cue].toneB;
        vab = g_SoundCueParams[cue].vab;
        prog = g_SoundCueParams[cue].program;
        tone = g_SoundCueParams[cue].toneA;
        baseVol = g_SoundCueParams[cue].volume;
    } else {
        vab = 0;
        if (g_SoundCueBank == 2) {
            tone2 = g_SoundCueParams2[cue].toneB;
            vab = g_SoundCueParams2[cue].vab;
            prog = g_SoundCueParams2[cue].program;
            tone = g_SoundCueParams2[cue].toneA;
            baseVol = g_SoundCueParams2[cue].volume;
        } else {
            prog = cue;
            baseVol = 0x80;
        }
    }

    volL = ScaleCueVolume(volL, baseVol);
    volR = ScaleCueVolume(volR, baseVol);

    if (g_SoundCueBank == 1) {
        for (i = 0; i < 6; i++) {
            busy[i] = SpuGetKeyStatus(voiceBits[i]);
        }
        for (i = 0; i < 6; i++) {
            if (busy[i] == 0) {
                result = (s16)SsUtKeyOnV((s16)(i + 0x12), g_SoundScale.vabIds[vab],
                                         (s16)prog, (s16)tone, 0x3C, 0,
                                         (s16)volL, (s16)volR);
                g_SpecialCueVoiceA = result;
                busy[i] = 1;
                break;
            }
        }
        for (i = 0; i < 6; i++) {
            if (busy[i] == 0) {
                result = (s16)SsUtKeyOnV((s16)(i + 0x12), g_SoundScale.vabIds[vab],
                                         (s16)prog, (s16)tone2, 0x3C, 0,
                                         (s16)volL, (s16)volR);
                g_SpecialCueVoiceB = result;
                busy[i] = 1;
                break;
            }
        }
    } else {
        result = (s16)SsUtKeyOn(g_SoundScale.vabIds[vab], prog, tone, 0x3C, 0,
                                volL, volR);
        g_SpecialCueVoiceA = result;
        result = (s16)SsUtKeyOn(g_SoundScale.vabIds[vab], prog, tone2,
                                0x3C, 0, volL, volR);
        g_SpecialCueVoiceB = result;
    }

    if (result < 0) {
        printf("%s", g_MsgTooManyVoices);
        return -1;
    }
    return result;
}


static s32 StartSingleSpecialCue(s32 cue, s32 volume) {
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
        result = (s16)SsUtKeyOnV(0x13, g_SoundScale.vabIds[vab],
                                 (s16)program, (s16)tone, 0x3C, 0,
                                 voiceVolume, voiceVolume);
        g_SpecialCueVoiceA = result;
    }

    g_ActiveSpecialCue = cue;
    return result;
}

static s32 StartSpecialCueVoice(s32 cue, s32 volumeLeft, s32 volumeRight) {
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

    if (SpuGetKeyStatus(g_SpecialVoiceBits[4]) == 0 ||
        cue == 0x3D || cue == 0x2B) {
        result = (s16)SsUtKeyOnV(0x16, g_SoundScale.vabIds[vab],
                                 (s16)prog, (s16)tone, 0x3C, 0,
                                 (s16)volumeLeft, (s16)volumeRight);
        g_SpecialCueVoiceA = result;
        result = (s16)SsUtKeyOnV(0x17, g_SoundScale.vabIds[vab],
                                 (s16)prog, (s16)(tone + 1), 0x3C, 0,
                                 (s16)volumeLeft, (s16)volumeRight);
        g_SpecialCueVoiceB = result;
    }

    return result;
}

void PlaySoundCue(s32 cue) {
    u32 specialCueRange;

    if (DiagnosticsEnabled("sound_cue_trace"))
        fprintf(stderr, "rage-port: sound cue=0x%02x\n", (unsigned)cue);

    if (g_SoundCueBank == 1) {
        if (cue >= 0) {
            if (cue >= 0x1E) {
                cue = 0x1D;
            }
        } else {
            cue = 0;
        }

        specialCueRange = cue - 0xF;
        if (specialCueRange < 3U) {
            if (cue != g_LastSpecialCueRequest) {
                g_LastSpecialCueRequest = cue;
                StartSingleSpecialCue(cue, 0x80);
            }
            return;
        }
        StartSoundCueVoice(cue, 0x80, 0x80);
        return;
    }

    if (g_SoundCueBank == 2) {
        if (cue >= 0) {
            if (cue >= 0x46) {
                cue = 0x45;
            }
        } else {
            cue = 0;
        }

        specialCueRange = cue - 0xF;
        if (specialCueRange < 3U) {
            if (cue != g_LastSpecialCueRequest) {
                g_LastSpecialCueRequest = cue;
                StartSingleSpecialCue(cue, 0x80);
            }
            return;
        }
        if (cue < 0x19) {
            StartSoundCueVoice(cue, 0x80, 0x80);
            return;
        }
        StartSpecialCueVoice(cue, 0x80, 0x80);
    }
}


/* Sets one engine-sound slot: scales `volume` by the global effect scale,
 * pushes it to the slot's voice, then re-pitches that voice to the tone at
 * g_SoundSlotTone[slot][toneIndex]. */
void SetSoundSlotTone(s32 slot, s32 bend, s32 volume, s32 toneIndex, u16 vabSlot) {
    s16 voice = (s16)(slot + 0xE);
    s32 scaledVolume = ClampVoiceVolume(
        volume * g_SoundScale.scale / 128);

    SsUtSetVVol(voice, scaledVolume, scaledVolume);
    SsUtPitchBend(voice, g_SoundScale.vabIds[(s16)vabSlot],
                  g_SoundSlotTone[slot][toneIndex], 0x3C, (s16)bend);
}
