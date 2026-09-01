#include "game/diagnostics.h"
#include "game/audio.h"
#include "psyq/snd.h"
#include "game/sound.h"

#include <stdio.h>
#include <stdlib.h>

/* `note` is what every caller passes as the MIDI note, always 0x3C. */
s32 StartSoundCueVoice(s32 cue, s32 note, s32 volL, s32 volR) {
    const s32 *voiceBits;
    s32 busy[6];
    s32 tone2;
    s32 vab;
    s32 prog;
    s32 tone;
    s32 baseVol;
    s32 scale;
    s32 scaled;
    s32 result = -1;
    (void)note;
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

    scaled = volL * baseVol;
    if (scaled < 0) {
        scaled += 0x7F;
    }
    scale = g_SoundScale.scale;
    scaled = (scaled >> 7) * scale;
    volL = scaled / 128;
    scaled = volR * baseVol;
    if (scaled < 0) {
        scaled += 0x7F;
    }
    scaled = (scaled >> 7) * scale;
    volR = scaled / 128;

    if (g_SoundCueBank == 1) {
        i = 0;
        do {
            busy[i] = SpuGetKeyStatus(voiceBits[i]);
            i++;
        } while (i < 6);
        i = 0;
        do {
            if (busy[i] == 0) {
                result = (s16)SsUtKeyOnV((s16)(i + 0x12), g_SoundScale.vabIds[vab],
                                         (s16)prog, (s16)tone, 0x3C, 0,
                                         (s16)volL, (s16)volR);
                g_SpecialCueVoiceA = result;
                busy[i] = 1;
                break;
            }
            i++;
        } while (i < 6);
        i = 0;
        do {
            if (busy[i] == 0) {
                result = (s16)SsUtKeyOnV((s16)(i + 0x12), g_SoundScale.vabIds[vab],
                                         (s16)prog, (s16)tone2, 0x3C, 0,
                                         (s16)volL, (s16)volR);
                g_SpecialCueVoiceB = result;
                busy[i] = 1;
                break;
            }
            i++;
        } while (i < 6);
    } else {
        result = (s16)SsUtKeyOn(g_SoundScale.vabIds[vab], prog, tone, 0x3C, 0,
                                volL, volR);
        result = (s16)SsUtKeyOn(g_SoundScale.vabIds[(g_SpecialCueVoiceA = result, vab)],
                                prog, tone2, 0x3C, 0, volL, volR);
        g_SpecialCueVoiceB = result;
    }

    if (result < 0) {
        printf("%s", g_MsgTooManyVoices);
        return -1;
    }
    return result;
}


s32 StartSingleSpecialCue(s32 cue, s32 volume) {
    s32 result = -1;
    s32 voiceVolume;
    s32 *handle;
    s32 value;
    s32 vab;
    s32 program;
    s32 tone;
    s32 scaled;
    s32 scaleValue;
    s32 current;

    handle = &g_SpecialCueVoiceA;
    g_SpecialCueVoiceB = result;
    *handle = result;
    current = g_ActiveSpecialCue;

    if (current != cue) {
        scaled = volume * g_SoundCueParams[cue].volume;
        vab = g_SoundCueParams[cue].vab;
        program = g_SoundCueParams[cue].program;
        tone = g_SoundCueParams[cue].toneA;
        if (scaled < 0) {
            scaled += 0x7F;
        }

        result = g_SoundScale.scale;
        value = scaled >> 7;
        value *= result;
        result = value;
        if (result < 0) {
            result += 0x7F;
        }

        scaleValue = g_SoundScale.vabIds[vab];
        program = (s16)program;
        tone = (s16)tone;
        voiceVolume = (result << 9) >> 16;
        result = (s16)SsUtKeyOnV(
            0x13,
            scaleValue,
            program,
            tone,
            0x3C,
            0,
            voiceVolume,
            voiceVolume);
        *handle = result;
    }

    g_ActiveSpecialCue = cue;
    return result;
}

s32 StartSpecialCueVoice(s32 cue, s32 volumeLeft, s32 volumeRight) {
    s32 id;
    s32 vab;
    s32 prog;
    s32 tone;
    s32 sx;
    s32 sy;
    s32 result = -1;
    s32 baseVol;
    s32 scale;
    s32 vx;
    s32 vy;
    s32 nextTone;

    id = cue;
    sy = volumeRight;
    baseVol = g_SoundCueParams2[id].volume;
    vab = g_SoundCueParams2[id].vab;
    prog = g_SoundCueParams2[id].program;
    tone = g_SoundCueParams2[id].toneA;

    vx = baseVol * volumeLeft;
    if (vx < 0) {
        vx += 0x7F;
    }
    scale = g_SoundScale.scale;
    sx = (vx >> 7) * scale;
    if (sx < 0) {
        sx += 0x7F;
    }
    vy = baseVol * sy;
    if (vy < 0) {
        vy += 0x7F;
    }
    sx >>= 7;
    volumeRight = (vy >> 7) * scale;
    sy = volumeRight / 128;

    if ((SpuGetKeyStatus(g_SpecialVoiceBits[4]) == 0) || (id == 0x3D) || (id == 0x2B)) {
        result = (s16)SsUtKeyOnV(
            0x16,
            g_SoundScale.vabIds[vab],
            (s16)prog,
            (s16)tone,
            0x3C,
            0,
            (s16)sx,
            (s16)sy);
        nextTone = tone + 1;
        nextTone = (s16)nextTone;
        result = (s16)SsUtKeyOnV(
            0x17,
            g_SoundScale.vabIds[(g_SpecialCueVoiceA = result, vab)],
            (s16)prog,
            nextTone,
            0x3C,
            0,
            (s16)sx,
            (s16)sy);
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
        StartSoundCueVoice(cue, 0x3C, 0x80, 0x80);
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
            StartSoundCueVoice(cue, 0x3C, 0x80, 0x80);
            return;
        }
        StartSpecialCueVoice(cue, 0x80, 0x80);
    }
}


/* Sets one engine-sound slot: scales `volume` by the global effect scale,
 * pushes it to the slot's voice, then re-pitches that voice to the tone at
 * g_SoundSlotTone[slot][toneIndex]. */
void SetSoundSlotTone(s32 slot, s32 bend, s32 volume, s32 toneIndex, u16 vabSlot) {
    s32 voice;
    s32 left;
    s32 right;
    s32 prod;
    s32 vab;
    s32 voiceCopy;

    prod = volume * g_SoundScale.scale;
    voice = slot + 0xE;
    voiceCopy = voice;
    vab = vabSlot;
    left = prod;
    if (prod < 0) {
        left = prod + 0x7F;
    }
    left >>= 7;
    right = left;
    if (right >= 0) {
        if (right >= 0x81) {
            left = 0x80;
        }
    } else {
        left = 0;
    }
    right = ClampVoiceVolume(right);
    SsUtSetVVol((s16)voiceCopy, left, right);
    voice = slot + 0xE;
    SsUtPitchBend((s16)voice, g_SoundScale.vabIds[(s16)vab], g_SoundSlotTone[slot][toneIndex], 0x3C, (s16)bend);
}
