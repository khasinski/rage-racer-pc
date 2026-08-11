#include "common.h"
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
    s32 values[2];
    s32 changed;
    s32 i;
    s32 *dst;
    s32 *src;
    s32 raw;
    s32 loopValue;
    s32 scale;
    s32 left;
    register s32 right asm("$6");
    register s32 voice asm("$4");
    s32 zeroArg;

    changed = 0;
    i = 0;
    dst = values;
    src = &g_PanVoiceVolumeL;
    do {
        loopValue = *src;
        if (loopValue < 2) {
            *dst = 0;
        } else {
            *dst = loopValue;
            changed = 1;
        }
        dst++;
        i++;
        src++;
    } while (i < 2);

    if (changed != 0) {
        raw = values[0];
        scale = g_SoundScale.scale;
        left = raw * scale;
        raw = values[1];
        if (left < 0) {
            left += 0x7F;
        }
        raw *= scale;
        if (raw < 0) {
            raw += 0x7F;
        }
        left >>= 7;
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
        if (g_PanVoiceActive == 0) {
            right = 0xF;
            voice = 0x15;
            asm volatile("" : : "r"(voice));
            raw = 0x3C;
            left = g_SoundScale.vabIds[0];
            zeroArg = 0;
            SsUtKeyOnV(voice, left, right, zeroArg, raw, 0, 0, 0);
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

    if (volume >= 0) {
        if (volume >= 0x80) {
            volume = 0x7F;
        }
    } else {
        volume = 0;
    }

    g_IndexedEffectIndex = index;
    if (index >= 0) {
        g_IndexedEffectVolume = volume;
        g_IndexedEffectPitch = phase;
    }
}

void UpdateIndexedEffectVoice(void) {
    s32 base;
    s32 center;
    s32 fine;
    s32 index;
    register s32 raw asm("$2");
    s32 product;
    s32 scale;
    register s32 left asm("$5");
    register s32 right asm("$6");
    register s32 voice asm("$4");

    raw = g_IndexedEffectIndexPrev;
    if (raw < 0) {
        index = g_IndexedEffectIndex;
        if (index < 0) {
            goto indexed_effect_done;
        }
            StartIndexedEffectVoice(g_IndexedEffects[index].tone);
            } else {
        index = g_IndexedEffectIndex;
        if (index < 0) {
            StopIndexedEffectVoice();
        } else if (index != raw) {
            StartIndexedEffectVoice(g_IndexedEffects[index].tone);
        }
    }

    raw = g_IndexedEffectIndex;
    if (raw >= 0) {
        index = raw;
        product = g_IndexedEffectVolume * g_IndexedEffects[index].volume;
        raw = g_IndexedEffectPitch;
        base = g_IndexedEffects[index].tone;
        center = raw >> 7;
        fine = raw & 0x7F;
        if (product < 0) {
            product += 0x7F;
        }
        raw = product >> 7;
        scale = g_SoundScale.scale;
        raw *= scale;
        left = raw;
        if (raw < 0) {
            left = raw + 0x7F;
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

        SsUtSetVVol(0x14, left, right);
        voice = 0x14;
        left = 0;
        asm volatile("" : : "r"(voice), "r"(left));
        right = (s16)base;
        raw = (s16)center;
        SsUtChangePitch(voice, left, right, 0x3C, 0, raw, fine);
    }

indexed_effect_done:
    g_IndexedEffectIndexPrev = g_IndexedEffectIndex;
}

/* The retail loop keeps i * sizeof(MusicChannel) in a register. */
#define CHANNEL(byteOffset) (*(MusicChannel *)((u8 *)g_MusicChannels + (byteOffset)))

void SetStereoSoundCue(s32 cue, s32 left, s32 right) {
    s32 count;
    s32 i;
    s32 loopTableOffset;
    s32 average;
    /* Load-bearing: removing this $v0 pin changes four linked words. */
    register s32 scaledLeft asm("$2");
    s32 scaledRight;
    s32 entryOffset;
    AudioRuntimeState *runtime;
    s32 currentA;
    s32 currentB;
    s32 matchValue;
    s32 flag;
    SoundModeEntry *base;
    SoundModeEntry *entry;
    SoundModeEntryAddress entryAddress;
    u32 cueIndex;

    if (cue >= 0) {
        if (cue >= 4) {
            cue = 3;
        }
    } else {
        cue = 0;
    }

    if (left >= 0) {
        if (left >= 0x80) {
            left = 0x7F;
        }
    } else {
        left = 0;
    }

    if (right >= 0) {
        if (right >= 0x80) {
            right = 0x7F;
        }
    } else {
        right = 0;
    }

    if ((left <= 0) && (right <= 0)) {
        left = g_MusicChannels[0].left.value;
        right = 0;
        if (left < 0) {
            if (g_MusicChannels[1].left.value < 0) {
                return;
            }
        }

        cueIndex = cue;
        if (cueIndex < 2) {
            if (left == g_SoundModes[0].slots[0].left) {
                currentB = g_MusicChannels[1].left.value;
                if (currentB == g_SoundModes[0].slots[1].left) {
                    goto found_match;
                }
            }
            if (left == g_SoundModes[1].slots[0].left) {
                currentB = g_MusicChannels[1].left.value;
                matchValue = g_SoundModes[1].slots[1].left;
                goto compare_mode_match;
            }
        } else {
            if (left == g_SoundModes[2].slots[0].left) {
                currentB = g_MusicChannels[1].left.value;
                if (currentB == g_SoundModes[2].slots[1].left) {
                    goto found_match;
                }
            }
            if (left == g_SoundModes[3].slots[0].left) {
                currentB = g_MusicChannels[1].left.value;
                matchValue = g_SoundModes[3].slots[1].left;
                goto compare_mode_match;
            }
        }
        goto after_match;
compare_mode_match:
        if (!(currentB != matchValue)) {
found_match:
        right = 1;
        }
after_match:

        if (right != 0) {
            /* Load-bearing: removing this $v0 pin changes 139 linked words. */
            s32 resetLoad;
            s32 resetCount;
            s32 inactiveValue;
            s32 activeValue;

            resetLoad = GetSoundModeAtByteOffset((cue * 3) << 3)->count;
            i = 0;
            if (resetLoad <= i) {
                return;
            }
            inactiveValue = -1;
            activeValue = 1;
            resetCount = resetLoad;
            do {
                g_MusicChannels[i].left.value = inactiveValue;
                g_MusicChannels[i].right.value = inactiveValue;
                g_MusicChannels[i].mode = activeValue;
                g_MusicChannels[i].volRight.value = 0;
                g_MusicChannels[i].volLeft.value = 0;
                i++;
            } while (i < resetCount);
        }
        return;
    }

    currentA = g_MusicChannels[0].left.value;
    if (currentA == GetSoundModeAtByteOffset((cue * 3) << 3)->slots[0].left) {
        currentB = g_MusicChannels[1].left.value;
        if (currentB == GetSoundModeAtByteOffset((cue * 3) << 3)->slots[1].left) {
            g_MusicChannels[0].mode = 2;
        } else {
            g_MusicChannels[0].mode = 0;
        }
    } else {
        g_MusicChannels[0].mode = 0;
    }

    i = 0;
    loopTableOffset = (cue * 3) << 3;
    cue = GetSoundModeAtByteOffset(loopTableOffset)->count;
    if (cue <= i) {
        return;
    }

    average = (left + right) / 2;
    count = cue;
    /* Load-bearing: removal changes five linked preheader words. */
    asm("" : "=r"(count) : "0"(count));
    base = g_SoundModes;
    entryOffset = loopTableOffset;
    entryAddress.pointer = base;
    entryAddress.bytes += entryOffset;
    entry = entryAddress.pointer;
    cue = 0;
    do {
        if (i != 0) {
            CHANNEL(cue).mode = CHANNEL(0).mode;
        }

        flag = g_StereoOutput;
        CHANNEL(cue).left.value = entry->slots[0].left;
        CHANNEL(cue).right.value = entry->slots[0].right;
        if (flag != 0) {
            currentB = GetSoundModeAtByteOffset(entryOffset)->factor;
            scaledLeft = left * currentB;
            if (scaledLeft < 0) {
                scaledLeft += 0x7F;
            }
            scaledLeft >>= 7;
            CHANNEL(cue).volLeft.updated = scaledLeft;
            scaledRight = right * currentB;
            entryAddress.pointer = entry;
            entryAddress.bytes += sizeof(SoundModeSlot);
            entry = entryAddress.pointer;
            if (scaledRight < 0) {
                scaledRight += 0x7F;
            }
            scaledRight >>= 7;
            CHANNEL(cue).volRight.updated = scaledRight;
            i++;
        } else {
            if ((scaledLeft = average * GetSoundModeAtByteOffset(entryOffset)->factor) < 0) {
                currentB = scaledLeft + 0x7F;
            } else {
                currentB = scaledLeft;
            }
            currentB >>= 7;
            CHANNEL(cue).volLeft.updated = currentB;
            SetMusicChannelWordUpdated(CHANNEL(cue).volRight, currentB);
            /* Load-bearing: removal changes eight linked scheduler words. */
            asm volatile("");
            entryAddress.pointer = entry;
            entryAddress.bytes += sizeof(SoundModeSlot);
            entry = entryAddress.pointer;
            i++;
        }
        cue += 0x18;
    } while (i < count);
}

#define UPDATE_BASIC_EFFECT_VOLUME()                                  \
    updateLeftAddress.wordPointer = &g_MusicChannels[0].volLeft.value; \
    updateLeftAddress.bytes += offset;                                \
    raw = *updateLeftAddress.wordPointer;                             \
    scale = g_SoundScale.scale;                                                \
    left = raw * scale;                                                \
    updateRightAddress.wordPointer = &g_MusicChannels[0].volRight.value; \
    updateRightAddress.bytes += offset;                               \
    raw = *updateRightAddress.wordPointer;                            \
    voice = i + 8;                                                     \
    if (left < 0) {                                                    \
        left += 0x7F;                                                  \
    }                                                                 \
    raw *= scale;                                                      \
    if (raw < 0) {                                                     \
        raw += 0x7F;                                                   \
    }                                                                 \
    left >>= 7;                                                        \
    right = raw >> 7;                                                  \
    if (left >= 0) {                                                   \
        if (left >= 0x81) {                                            \
            left = 0x80;                                               \
        }                                                             \
    } else {                                                          \
        left = 0;                                                      \
    }                                                                 \
    if (right >= 0) {                                                  \
        if (right >= 0x81) {                                           \
            right = 0x80;                                              \
        }                                                             \
    } else {                                                          \
        right = 0;                                                     \
    }                                                                 \
    SsUtSetVVol((s16)voice, (s16)left, (s16)right);                   \
    *state = neg

#define START_BASIC_EFFECT_VOLUME()                                   \
    startLeftAddress.wordPointer = &g_MusicChannels[0].volLeft.value; \
    startLeftAddress.bytes += offset;                                 \
    raw = *startLeftAddress.wordPointer;                              \
    scale = g_SoundScale.scale;                                                \
    left = raw * scale;                                                \
    raw = i + 8;                                                       \
    asm("" : "=r"(raw) : "0"(raw));                                    \
    voice = raw;                                                       \
    startRightAddress.wordPointer = &g_MusicChannels[0].volRight.value; \
    startRightAddress.bytes += offset;                                \
    raw = *startRightAddress.wordPointer;                             \
    if (left < 0) {                                                    \
        left += 0x7F;                                                  \
    }                                                                 \
    raw *= scale;                                                      \
    if (raw < 0) {                                                     \
        raw += 0x7F;                                                   \
    }                                                                 \
    left >>= 7;                                                        \
    right = raw >> 7;                                                  \
    if (left >= 0) {                                                   \
        if (left >= 0x81) {                                            \
            left = 0x80;                                               \
        }                                                             \
    } else {                                                          \
        left = 0;                                                      \
    }                                                                 \
    if (right >= 0) {                                                  \
        if (right >= 0x81) {                                           \
            right = 0x80;                                              \
        }                                                             \
    } else {                                                          \
        right = 0;                                                     \
    }                                                                 \
    SsUtSetVVol((s16)voice, (s16)left, (s16)right);                   \
    *state = neg

void UpdateBasicEffectVoices(void) {
    s32 offset;
    s32 *state;
    s32 i;
    s32 voicePacked;
    s32 neg;
    s32 raw;
    s32 scale;
    s32 voice;
    s32 left;
    s32 right;
    MusicChannelAddress leftToneAddress;
    MusicChannelAddress rightToneAddress;
    MusicChannelAddress updateLeftAddress;
    MusicChannelAddress updateRightAddress;
    MusicChannelAddress startLeftAddress;
    MusicChannelAddress startRightAddress;

    i = 0;
    neg = -1;
    state = &g_MusicChannels[0].mode;
    voicePacked = 0x80000;
    offset = 0;
    do {
        switch (*state) {
        case 0:
            leftToneAddress.halfwordPointer = &g_MusicChannels[0].left.half[0];
            leftToneAddress.bytes += offset;
            rightToneAddress.halfwordPointer = &g_MusicChannels[0].right.half[0];
            rightToneAddress.bytes += offset;
            SsUtKeyOnV(voicePacked >> 16, g_SoundScale.vabIds[0],
                          *leftToneAddress.halfwordPointer,
                          *rightToneAddress.halfwordPointer, 0x3C, 0, 0, 0);
            START_BASIC_EFFECT_VOLUME();
            break;
        case 2:
            UPDATE_BASIC_EFFECT_VOLUME();
            break;
        case 1:
            SsUtKeyOffV(voicePacked >> 16);
            *state = neg;
            break;
        }
        state += sizeof(MusicChannel) / sizeof(*state);
        voicePacked += 0x10000;
        i++;
        offset += sizeof(MusicChannel);
    } while (i < 2);
}
