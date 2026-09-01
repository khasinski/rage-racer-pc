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

/* Are the two music channels sitting exactly on this sound mode's pair? */
static int MusicChannelsOnMode(s32 mode, s32 channelLeft) {
    return channelLeft == g_SoundModes[mode].slots[0].left &&
           g_MusicChannels[1].left.value == g_SoundModes[mode].slots[1].left;
}

/* The retail loop keeps i * sizeof(MusicChannel) in a register. */
#define CHANNEL(byteOffset) (*(MusicChannel *)((u8 *)g_MusicChannels + (byteOffset)))

void SetStereoSoundCue(s32 cue, s32 left, s32 right) {
    s32 count;
    s32 i;
    s32 loopTableOffset;
    s32 average;
    s32 scaledLeft;
    s32 scaledRight;
    s32 entryOffset;
    s32 currentA;
    s32 currentB;
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

    left = ClampCueLevel(left);
    right = ClampCueLevel(right);

    if ((left <= 0) && (right <= 0)) {
        left = g_MusicChannels[0].left.value;
        right = 0;
        if (left < 0) {
            if (g_MusicChannels[1].left.value < 0) {
                return;
            }
        }

        /*
         * The two music channels are already sitting on one of this cue's two
         * sound modes, so the cue is redundant. Cues below 2 use modes 0 and
         * 1, the rest 2 and 3.
         */
        cueIndex = cue < 2 ? 0 : 2;
        right = MusicChannelsOnMode(cueIndex, left) ||
                        MusicChannelsOnMode(cueIndex + 1, left)
                    ? 1
                    : 0;

        if (right != 0) {
            /* Already on this cue's mode: hand every channel it owns back to
             * the mode and leave the cue alone. */
            s32 count = GetSoundModeAtByteOffset((cue * 3) << 3)->count;

            for (i = 0; i < count; i++) {
                g_MusicChannels[i].left.value = -1;
                g_MusicChannels[i].right.value = -1;
                g_MusicChannels[i].mode = 1;
                g_MusicChannels[i].volRight = 0;
                g_MusicChannels[i].volLeft = 0;
            }
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
            scaledLeft /= 128;
            CHANNEL(cue).volLeft = scaledLeft;
            scaledRight = right * currentB;
            entryAddress.pointer = entry;
            entryAddress.bytes += sizeof(SoundModeSlot);
            entry = entryAddress.pointer;
            scaledRight /= 128;
            CHANNEL(cue).volRight = scaledRight;
            i++;
        } else {
            if ((scaledLeft = average * GetSoundModeAtByteOffset(entryOffset)->factor) < 0) {
                currentB = scaledLeft + 0x7F;
            } else {
                currentB = scaledLeft;
            }
            currentB >>= 7;
            CHANNEL(cue).volLeft = currentB;
            CHANNEL(cue).volRight = currentB;
            /* Load-bearing: removal changes eight linked scheduler words. */
            
            entryAddress.pointer = entry;
            entryAddress.bytes += sizeof(SoundModeSlot);
            entry = entryAddress.pointer;
            i++;
        }
        cue += 0x18;
    } while (i < count);
}

void UpdateBasicEffectVoices(void) {
    s32 i;

    for (i = 0; i < 2; i++) {
        MusicChannel *channel = &g_MusicChannels[i];
        s16 voice = (s16)(8 + i);

        switch (channel->mode) {
        case 0:
            SsUtKeyOnV(voice, g_SoundScale.vabIds[0], channel->left.half[0],
                       channel->right.half[0], 0x3C, 0, 0, 0);
            /* Fall through: a newly keyed voice needs the same volume update. */
        case 2:
            SsUtSetVVol(
                voice,
                ClampVoiceVolume(channel->volLeft * g_SoundScale.scale / 128),
                ClampVoiceVolume(channel->volRight * g_SoundScale.scale / 128));
            channel->mode = -1;
            break;
        case 1:
            SsUtKeyOffV(voice);
            channel->mode = -1;
            break;
        }
    }
}
