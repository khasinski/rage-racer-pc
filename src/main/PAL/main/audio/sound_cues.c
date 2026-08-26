#include "common.h"
#include "game/audio.h"
#include "psyq/snd.h"
#include "game/sound.h"

#ifdef __psyz
#include <stdio.h>
#include <stdlib.h>
#endif

void SetPitchedSoundCue(s32 bank, s32 pitch, s32 volume) {
    s32 count;
    s32 loopCount;
    s32 i;
    /* Load-bearing GCC 2.6.3 register roles for the table loops. */
    register s32 tblOff asm("$3");
    register s32 loopTblOff asm("$8");
    s32 scaled;
    s32 scaleValue;
    s32 cueValue;
    s32 toneValue;
    s32 hasActiveVoice;
    s32 ok;
    s32 bankIndex;
    s32 active;
    s32 inactive;
    s32 defaultPitch;
    register const EffectCueBankEntry *cueCursor asm("$7");
    const EffectCueBank *tableBase;
    s32 *stateBase;

    if (bank >= 0) {
        if (bank >= 3) {
            bank = 2;
        }
    } else {
        bank = 0;
    }
    if (volume >= 0) {
        if (volume >= 0x80) {
            volume = 0x7F;
        }
    } else {
        volume = 0;
    }

    switch (bank) {
    case 0:
        if (volume <= 0) {
            s32 resetIndex;

            resetIndex = 0;
            if ((g_EffectVoices[0].note.value >= 0) || (g_EffectVoices[1].note.value >= 0)) {
                count = g_EffectCueTable[0].voiceCount;
                if (count > 0) {
                    active = 1;
                    inactive = -1;
                    defaultPitch = 0x1E00;
                    volume = count;
                    do {
                        g_EffectVoices[resetIndex].state = active;
                        g_EffectVoices[resetIndex].note.value = inactive;
                        g_EffectVoices[resetIndex].tone = inactive;
                        g_EffectVoices[resetIndex].pitch.value = defaultPitch;
                        g_EffectVoices[resetIndex].volume = 0;
                        resetIndex++;
                    } while (resetIndex < volume);
                }
            }
        } else {
            if ((g_EffectVoices[0].note.value == g_EffectCueTable[0].programs[0].note) &&
                (g_EffectVoices[1].note.value == g_EffectCueTable[0].programs[1].note)) {
                g_EffectVoices[0].state = 2;
            } else {
                g_EffectVoices[0].state = 0;
            }
            bankIndex = bank * 3;
            tblOff = bankIndex * 8;
            count = g_EffectCueTable[bank].voiceCount;
            i = 0;
            if (count > i) {
                stateBase = &g_EffectVoices[0].state;
                loopCount = count;
                tableBase = g_EffectCueTable;
                loopTblOff = tblOff;
                {
                    EffectCueBankAddress cueAddress;

                    cueAddress.pointer = tableBase;
                    cueAddress.bytes += loopTblOff;
                    cueCursor = cueAddress.entryPointer;
                }
                do {
                    EffectCueBankAddress scaleAddress;

                    if (i != 0) {
                        g_EffectVoices[i].state = *stateBase;
                    }
                    scaleAddress.pointer = g_EffectCueTable;
                    scaleAddress.bytes += loopTblOff;
                    scaleValue = scaleAddress.pointer->volumeScale;
                    scaled = volume * scaleValue;
                    cueValue = cueCursor[1].program.note;
                    g_EffectVoices[i].note.value = cueValue;
                    toneValue = cueCursor[1].program.tone;
                    g_EffectVoices[i].pitch.value = pitch;
                    g_EffectVoices[i].tone = toneValue;
                    cueCursor++;
                    if (scaled < 0) {
                        scaled += 0x7F;
                    }
                    g_EffectVoices[i].volume = scaled >> 7;
                    i++;
                } while (i < loopCount);
            }
        }
        break;

    case 1:
    case 2:
        if (volume <= 0) {
            hasActiveVoice = g_EffectVoices[2].note.value >= 0;
            ok = 0;
            if (hasActiveVoice || (g_EffectVoices[3].note.value >= 0)) {
                if (bank == 1) {
                    if (g_EffectVoices[2].note.value == g_EffectCueTable[1].programs[0].note) {
                        ok = g_EffectVoices[3].note.value == g_EffectCueTable[1].programs[1].note;
                    }
                } else if ((bank == 2) &&
                           (g_EffectVoices[2].note.value == g_EffectCueTable[2].programs[0].note) &&
                           (g_EffectVoices[3].note.value == g_EffectCueTable[2].programs[1].note)) {
                    ok = 1;
                }
                if (ok != 0) {
                    count = g_EffectCueTable[bank].voiceCount;
                    i = 0;
                    if (count > i) {
                        active = 1;
                        inactive = -1;
                        defaultPitch = 0x1E00;
                        volume = count;
                        do {
                            g_EffectVoices[i + 2].state = active;
                            g_EffectVoices[i + 2].note.value = inactive;
                            g_EffectVoices[i + 2].tone = inactive;
                            g_EffectVoices[i + 2].pitch.value = defaultPitch;
                            g_EffectVoices[i + 2].volume = 0;
                            i++;
                        } while (i < volume);
                    }
                }
            }
        } else {
            if ((g_EffectVoices[2].note.value ==
                 g_EffectCueTable[bank].programs[0].note) &&
                (g_EffectVoices[3].note.value ==
                 g_EffectCueTable[bank].programs[1].note)) {
                g_EffectVoices[2].state = 2;
                tblOff = bank * 2;
            } else {
                g_EffectVoices[2].state = 0;
                tblOff = bank * 2;
            }
            tblOff = (tblOff + bank) * 8;
            bankIndex = bank * 3;
            tblOff = bankIndex * 8;
            count = g_EffectCueTable[bank].voiceCount;
            i = 0;
            if (count > i) {
                stateBase = &g_EffectVoices[2].state;
                loopCount = count;
                tableBase = g_EffectCueTable;
                loopTblOff = tblOff;
                {
                    EffectCueBankAddress cueAddress;

                    cueAddress.pointer = tableBase;
                    cueAddress.bytes += loopTblOff;
                    cueCursor = cueAddress.entryPointer;
                }
                do {
                    EffectCueBankAddress scaleAddress;

                    if (i != 0) {
                        g_EffectVoices[i + 2].state = *stateBase;
                    }
                    scaleAddress.pointer = g_EffectCueTable;
                    scaleAddress.bytes += loopTblOff;
                    scaleValue = scaleAddress.pointer->volumeScale;
                    scaled = volume * scaleValue;
                    cueValue = cueCursor[1].program.note;
                    g_EffectVoices[i + 2].note.value = cueValue;
                    toneValue = cueCursor[1].program.tone;
                    g_EffectVoices[i + 2].pitch.value = pitch;
                    g_EffectVoices[i + 2].tone = toneValue;
                    cueCursor++;
                    if (scaled < 0) {
                        scaled += 0x7F;
                    }
                    g_EffectVoices[i + 2].volume = scaled >> 7;
                    i++;
                } while (i < loopCount);
            }
        }
        break;
    }
}

/* Loop over the 4 effect voices (indices 10..13). `voice` is kept in the
 * compiler's scaled (<<16) representation of the short voice number, exactly
 * as GCC materialises it, and read back with `voice >> 16`. */
#define VOLPITCH()                                                    \
    svArg = voiceCopy;                                                \
                               \
    prod = GetEffectVoiceAtByteOffset(offset)->volume * g_SoundScale.scale; \
    left = prod;                                                      \
    if (prod < 0) {                                                   \
        left = prod + 0x7F;                                           \
    }                                                                 \
    left >>= 7;                                                       \
    right = left;                                                     \
    if (right >= 0) {                                                 \
        if (right >= 0x81) {                                          \
            left = 0x80;                                              \
        }                                                             \
    } else {                                                          \
        left = 0;                                                     \
    }                                                                 \
    if (right >= 0) {                                                 \
        if (right >= 0x81) {                                          \
            right = 0x80;                                             \
        }                                                             \
    } else {                                                          \
        right = 0;                                                    \
    }                                                                 \
    SsUtSetVVol((s16)svArg, left, right);                             \
    SsUtChangePitch(voice >> 16, 0, *f0Ptr, 0x3C, 0,                  \
                    (s16)(pitchPtr->value >> 7), pitchPtr->half.fraction & 0x7F); \
    *statePtr = neg

void UpdateEffectVoiceStates(void) {
    EffectVoiceAddress cursorAddress;
    EffectVoiceAddress endAddress;
    EffectVoiceAddress toneAddress;
    s32 *statePtr;
    EffectVoicePitch *pitchPtr;
    s16 *f0Ptr;
    s32 offset;
    s32 voiceCopy;
    s32 neg;
    s32 svArg;
    register s32 left asm("$5");
    s32 right;
    s32 prod;
    s32 voice;
    s32 state;

    neg = -1;
    statePtr = &g_EffectVoices[0].state;
    voice = 10 << 16;
    voiceCopy = 10;
    pitchPtr = GetEffectVoicePitchFromState(statePtr);
    f0Ptr = GetEffectVoiceHalfwordsFromState(statePtr) - 4;
    offset = 0;
    do {
        state = *statePtr;
        switch (state) {
        case 0:
            toneAddress.pointer = g_EffectVoices;
            toneAddress.bytes += offset;
            SsUtKeyOnV(voice >> 16, g_SoundScale.vabIds[0], *f0Ptr,
                          (s16)toneAddress.pointer->tone,
                          0x3C, 0, 0, 0);
            VOLPITCH();
            break;
        case 2:
            VOLPITCH();
            break;
        case 1:
            SsUtKeyOffV(voice >> 16);
            *statePtr = neg;
            break;
        }
        statePtr += sizeof(EffectVoice) / sizeof(*statePtr);
        voice += 1 << 16;
        voiceCopy++;
        pitchPtr += sizeof(EffectVoice) / sizeof(*pitchPtr);
        f0Ptr += sizeof(EffectVoice) / sizeof(*f0Ptr);
        offset += sizeof(EffectVoice);
        cursorAddress.wordPointer = statePtr;
        endAddress.wordPointer = &g_EffectVoices[4].state;
    } while (cursorAddress.value < endAddress.value);
}


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
    if (scaled < 0) {
        scaled += 0x7F;
    }
    volL = scaled >> 7;
    scaled = volR * baseVol;
    if (scaled < 0) {
        scaled += 0x7F;
    }
    scaled = (scaled >> 7) * scale;
    if (scaled < 0) {
        scaled += 0x7F;
    }
    volR = scaled >> 7;

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
    if (volumeRight < 0) {
        volumeRight += 0x7F;
    }
    sy = volumeRight >> 7;

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

#ifdef __psyz
    if (getenv("RAGE_PORT_SOUND_CUE_TRACE") != NULL)
        fprintf(stderr, "rage-port: sound cue=0x%02x\n", (unsigned)cue);
#endif

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
    register s32 left asm("$5");
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
    if (right >= 0) {
        if (right >= 0x81) {
            right = 0x80;
        }
    } else {
        right = 0;
    }
    SsUtSetVVol((s16)voiceCopy, left, right);
    voice = slot + 0xE;
    SsUtPitchBend((s16)voice, g_SoundScale.vabIds[(s16)vab], g_SoundSlotTone[slot][toneIndex], 0x3C, (s16)bend);
}
