#include "common.h"
#include "game/audio.h"
#include "game/sound.h"
#include "psyq/snd.h"
#include "game/car.h"

s32 InterpolateAudioParameter(s32 parameter, s32 position, s32 bank) {
    s32 value = position;
    s32 index;
    s32 index_offset;
    s32 *base = g_EngineSoundCurves[0][0].positions;
    s32 row_offset;
    s32 bank_offset;
    s32 *base_minus;
    s32 *entry;
    s32 *scan;
    s32 *lower_position;
    s32 lower_position_value;
    s32 lower_value_value;
    s32 upper_value_value;
    s32 numerator;
    s32 denominator;
    s32 raw_result;
    s32 result;
    EngineSoundCurveAddress curveAddress;
    EngineSoundCurveAddress entryAddress;
    EngineSoundCurveAddress denominatorAddress;
    EngineSoundCurveAddress lowerValueAddress;
    EngineSoundCurveAddress upperValueAddress;

    index = 1;
    row_offset = (parameter * 9) << 3;
    bank_offset = (((bank * 7) * 4) - bank) << 5;
    bank = row_offset + bank_offset;
    entryAddress.pointer = base;
    entryAddress.bytes += bank;
    entry = entryAddress.pointer;
    scan = entry + 1;
    while (index < 9) {
        raw_result = *scan;
        if (value < raw_result) {
            break;
        }
        index++;
        scan++;
    }

    base_minus = base - 1;
    index_offset = index * 4;
    curveAddress.pointer = base_minus;
    curveAddress.value = index_offset + curveAddress.value;
    curveAddress.bytes += bank;
    lower_position = curveAddress.pointer;
    lowerValueAddress.pointer = base + 8;
    lowerValueAddress.value = index_offset + lowerValueAddress.value;
    lowerValueAddress.bytes += bank;
    upperValueAddress.pointer = base + 9;
    upperValueAddress.value = bank + upperValueAddress.value;
    upperValueAddress.value = index_offset + upperValueAddress.value;
    lower_value_value = *lowerValueAddress.pointer;
    upper_value_value = *upperValueAddress.pointer;
    lower_position_value = *lower_position;
    numerator =
        (upper_value_value - lower_value_value) *
        (value - lower_position_value);
    denominatorAddress.pointer = entry;
    denominatorAddress.value = index_offset + denominatorAddress.value;
    denominator = *denominatorAddress.pointer - lower_position_value;
    raw_result = numerator / denominator + lower_value_value;

    if (raw_result >= 0) {
        result = raw_result;
        if (result >= 0x80) {
            result = 0x7F;
        }
    } else {
        result = 0;
    }

    return result;
}

void UpdateLoadedAudioVoices(s32 value, s32 bank) {
    s32 odd_parameter;
    s32 index;
    s32 second;
    s32 scaled;
    s32 *scale_base;
    s32 *slot;
    s32 *slot_base;
    s32 first;

    value = ((value * 5) << 11) / *(scale_base = &g_EngineSoundState.maxRpm);

    if (bank != g_EngineSoundState.bank) {
        index = 0;
        slot = scale_base + 1;
        do {
            if (*slot++ != 0 &&
                g_SoundSlotTone[index][0] != g_SoundSlotTone[index][1]) {
                PlaySoundSlotVoice(index, bank, 3);
            }
            index++;
        } while (index < 6);
        g_EngineSoundState.bank = bank;
    }

    index = 0;
    odd_parameter = 1;
    scale_base = (slot_base = g_EngineSoundState.slotActive);
    slot = scale_base;
    do {
        if (*slot != 0) {
            first = InterpolateAudioParameter(index * 2, value, bank);
            second = InterpolateAudioParameter(odd_parameter, value, bank);
            scaled = second * slot_base[6];
            if (scaled < 0) {
                scaled += 0x7F;
            }
            SetSoundSlotTone(index, first, scaled >> 7, bank, 3);
        }
        odd_parameter += 2;
        index++;
        slot++;
    } while (index < 6);

    g_EngineSoundState.position = value;
    ApplyPanVoiceVolume();
    UpdateBasicEffectVoices();
    UpdateIndexedEffectVoice();
    UpdateEffectVoiceStates();
}

void SetDefaultReverbDepth(void) {
    SetReverbDepth(0x28, 0x28);
}

void InitSequenceAudio(void) {
    _SsVmInit(0);
    SsSetVoiceCount(0x12);
    SetReverbDepth(0x28, 0x28);
    g_ReverbFadeStep = 0;
    RefreshSequenceVolumeScale();
}

void InitEffectVoiceRuntime(void) {
    s32 i;

    _SsVmInit(0);
    SsSetVoiceCount(8);

    {
        s32 neg;

        i = 0;
        neg = -1;
        for (; i < 2; i++) {
            g_MusicChannels[i].mode = neg;
            g_MusicChannels[i].left.value = neg;
            g_MusicChannels[i].right.value = neg;
            g_MusicChannels[i].volLeft.value = 0;
            /* Retail reaches this field as
             * &g_AudioLoadedSlotMask + 0x78 + i * sizeof(MusicChannel).
             * That only works while independently named PS1 globals retain
             * their original contiguous addresses. Name the actual field so
             * the same game state is updated on 32- and 64-bit hosts. */
            g_MusicChannels[i].volRight.value = 0;
        }
    }

    {
        s32 neg;
        s32 value;

        i = 0;
        neg = -1;
        value = 0x1E00;
        for (; i < 4; i++) {
            g_EffectVoices[i].state = neg;
            g_EffectVoices[i].note.value = neg;
            g_EffectVoices[i].tone = neg;
            g_EffectVoices[i].pitch.value = value;
            g_EffectVoices[i].volume = 0;
        }
    }

    {
        s32 value;

        value = -1;
        g_PanVoiceVolumeR = value;
        g_PanVoiceVolumeL = value;
        g_IndexedEffectIndexPrev = value;
        g_IndexedEffectIndex = value;
        value = 0x1E00;
        g_PanVoiceActive = 0;
        g_IndexedEffectPitch = value;
    }

    SetEffectVoicesEnabled(1);
    SetReverbPreset(2, 0, 0);
    SetLoadedTableVolumeScale(g_CarSoundVolumeScales[GetOwnedCarAssetIndex(g_PlayerCarIndex)]);
}

void RestoreReverbDepth(s32 enabled) {
    if (enabled != 0) {
        SetReverbDepth(g_ReverbDepthL, g_ReverbDepthR);
    } else {
        SetReverbDepth(0, 0);
    }
}

void ForcePanVoiceEnabled(s32 enabled) {
    s32 values[2];
    s32 i;
    s32 *src;
    s32 *dst;
    s32 scale;
    s32 raw;
    register s32 voice asm("$4");
    s32 left;
    register s32 right asm("$6");
    s32 zeroArg;
    register s32 unused asm("$16");

    i = 0;
    dst = values;
    src = &g_PanVoiceVolumeL;
    do {
        if (*src < 2) {
            *dst = 0;
        } else {
            *dst = *src;
        }
        dst++;
        i++;
        src++;
    } while (i < 2);

    if (enabled != 0) {
        raw = values[0];
        scale = g_SoundScale.scale;
        left = raw * scale;
        raw = values[1];
        if (left < 0) {
            left += 0x7F;
        }
        unused = 0;
        asm volatile("" : : "r"(unused));
        raw *= scale;
        left >>= 7;
        if (raw < 0) {
            raw += 0x7F;
        }
        right = raw >> 7;

        if (left >= 0) {
            if (left >= 0x81) {
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

        SsUtSetVVol(0x15, left, right);
        voice = 0x15;
        right = 0xF;
        asm volatile("" : : "r"(voice), "r"(right));
        raw = 0x3C;
        left = g_SoundScale.vabIds[0];
        zeroArg = 0;
        SsUtKeyOnV(voice, left, right, zeroArg, raw, 0, 0, 0);
    } else {
        SsUtKeyOffV(0x15);
    }
}
