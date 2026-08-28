#include "common.h"
#include "game/audio.h"
#include "game/sound.h"
#include "psyq/snd.h"

void ForceBasicEffectVoicesEnabled(s32 enabled) {
    s32 voicePacked;
    s32 voice;
    s32 i;
    s32 unused;
    s32 raw;
    s32 scale;
    s32 left;
    s32 right;
    s32 voiceArg;
    s32 zeroArg;

    unused = 0;
    i = 0;
    voicePacked = 0x80000;
    voice = 8;
    do {
        if (enabled != 0) {
            voiceArg = voicePacked >> 16;
            raw = 0x3C;
            left = g_SoundScale.vabIds[0];
            right = g_MusicChannels[i].left.half[0];
            zeroArg = 0;
            SsUtKeyOnV(voiceArg, left, right, zeroArg, raw, 0, 0, 0);
            asm volatile("" : : "r"(unused));

            raw = g_MusicChannels[i].volLeft.value;
            scale = g_SoundScale.scale;
            left = raw * scale;
            raw = g_MusicChannels[i].volRight.value;
            voiceArg = voice;
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

            SsUtSetVVol((s16)voiceArg, left, right);
        } else {
            SsUtKeyOffV(voicePacked >> 16);
        }

        voicePacked += 0x10000;
        voice++;
        i++;
    } while (i < 2);
}

void ForceIndexedEffectVoiceEnabled(s32 enabled) {
    s32 base;
    s32 center;
    s32 fine;
    s32 index;
    s32 raw;
    s32 product;
    s32 scale;
    s32 left;
    s32 right;
    s32 voice;

    if (enabled != 0) {
        index = g_IndexedEffectIndexPrev;
        if (index < 0) {
            return;
        }
        StartIndexedEffectVoice(g_IndexedEffects[index].tone);
    } else {
        StopIndexedEffectVoice();
    }

    raw = g_IndexedEffectIndexPrev;
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
}

void ForcePitchEffectVoicesEnabled(s32 enabled) {
    EffectVoiceAddress cursorAddress;
    EffectVoiceAddress endAddress;
    s32 voicePacked;
    s32 voice;
    EffectVoicePitch *pitchCursor;
    EffectVoiceNote *noteCursor;
    s32 offset;
    s32 state;
    s32 raw;
    s32 scale;
    s32 left;
    s32 right;
    s32 voiceArg;
    s32 keyTone;

    state = enabled;
    voicePacked = 0xA0000;
    voice = 0xA;
    pitchCursor = &g_EffectVoices[0].pitch;
    noteCursor = &g_EffectVoices[0].note;
    offset = 0;
    do {
        if (state != 0) {
            voiceArg = voicePacked >> 16;
            left = g_SoundScale.vabIds[0];
            right = noteCursor->half.value;
            keyTone = (s16)GetEffectVoiceAtByteOffset(offset)->tone;
            raw = 0x3C;
            SsUtKeyOnV(voiceArg, left, right, keyTone, raw, 0, 0, 0);

            scale = GetEffectVoiceAtByteOffset(offset)->volume;
            asm volatile("" : : "r"(scale));
            raw = g_SoundScale.scale;
            raw = scale * raw;
            voiceArg = voice;
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

            SsUtSetVVol((s16)voiceArg, left, right);

            right = noteCursor->half.value;
            voiceArg = voicePacked >> 16;
            SsUtChangePitch(voiceArg, 0, right, 0x3C, 0,
                            (pitchCursor->value << 9) >> 16,
                            pitchCursor->half.fraction & 0x7F);
        } else {
            SsUtKeyOffV(voicePacked >> 16);
        }

        voicePacked += 0x10000;
        voice++;
        pitchCursor += sizeof(EffectVoice) / sizeof(*pitchCursor);
        noteCursor += sizeof(EffectVoice) / sizeof(*noteCursor);
        offset += 0x14;
        cursorAddress.pitchPointer = pitchCursor;
        endAddress.wordPointer = &g_ReverbFadeStep;
    } while (cursorAddress.value < endAddress.value);
}

void ForceSoundSlotVoicePlayback(s32 enabled) {
    s32 saved = enabled;
    s32 i;
    s32 *base;
    s32 *active;
    s32 odd;
    s32 first;
    s32 second;
    s32 factor;
    s32 scaled;
    s32 callSlot;
    s32 callBend;
    s32 callTone;

    SetSoundSlotVoicesEnabled(enabled);

    i = 0;
    if (saved != 0) {
        base = g_EngineSoundState.slotActive;
        active = base;
        saved = 0;
        do {
            if (*base++ != 0 && g_SoundSlotTone[i][0] != g_SoundSlotTone[i][1]) {
                PlaySoundSlotVoice(i, active[-3], 3);
            }
            i++;
            saved += 4;
        } while (i < 6);

        i = 0;
        odd = 1;
        base = g_EngineSoundState.slotActive;
        active = base;
        do {
            if (*active != 0) {
                first = InterpolateAudioParameter(i * 2, base[-4], base[-3]);
                second = InterpolateAudioParameter(odd, base[-4], base[-3]);
                factor = base[6];
                scaled = second * factor;
                if (scaled < 0) {
                    scaled += 0x7F;
                }
                callSlot = i;
                callBend = first;
                scaled >>= 7;
                callTone = base[-3];
                SetSoundSlotTone(callSlot, callBend, scaled, callTone, 3);
            }
            odd += 2;
            i++;
            active++;
        } while (i < 6);
    }
}

void ForceAllEffectVoicesEnabled(s32 enabled) {
    ForcePanVoiceEnabled(enabled);
    ForceBasicEffectVoicesEnabled(enabled);
    ForceIndexedEffectVoiceEnabled(enabled);
    ForcePitchEffectVoicesEnabled(enabled);
    ForceSoundSlotVoicePlayback(enabled);
}
