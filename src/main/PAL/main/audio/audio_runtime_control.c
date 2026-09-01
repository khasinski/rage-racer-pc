#include "game/audio.h"
#include "game/sound.h"
#include "game/state.h"
#include "game/work_buffer.h"
#include "psyq/snd.h"
#include "timing_control.h"

/* libsnd counts musical time in ticks of a sixtieth of a second, which is what
 * retail asked for: SsSetTickMode(SS_TICK60) left VBLANK_MINUS at sixty and
 * drove SsSeqCalledTbyT from a counter interrupt at sixty hertz on a PAL
 * console as well, so the music kept its tempo while the picture ran at fifty.
 * The host has no such interrupt and services the sequencer from the game
 * loop, so it has to pay the same sixty ticks a second out of frames that
 * arrive at fifty. */
#define SEQUENCE_TICK_HZ 60

void TickSequenceAudio(void) {
    /* Ticks the sequencer is owed, in units of one game frame. */
    static s32 tickCredit;

    if (g_SceneId == 0xC) {
        SpuVmDamperStep();
    } else {
        s32 frameHz = TimingBaseHz();

        tickCredit += SEQUENCE_TICK_HZ;
        while (tickCredit >= frameHz) {
            tickCredit -= frameHz;
            SsSeqCalledTbyT();
        }
        if (g_SeqVolumeFadeStep != 0) {
            UpdateSequenceFadeOut();
        }
        /* libsnd batches voice register writes. On PS1 its sound interrupt
         * flushed the final key-off; the host drives this service directly. */
        SpuVmDamperStep();
    }
}

void SetReverbDepth(s32 left, s32 right) {
    left = ClampCueLevel(left);
    right = ClampCueLevel(right);

    g_ReverbDepthL = left;
    g_ReverbDepthR = right;
    SsUtSetReverbDepth((s16)left, (s16)right);
}

void SetDefaultReverbDepth(void) {
    SetReverbDepth(0x28, 0x28);
}

void SetReverbPreset(s32 type, s32 left, s32 right) {
    s32 tempLeft;
    s32 tempRight;
    u32 presetIndex;

    if (left >= 0) {
        tempLeft = left;
        if (tempLeft >= 0x80) {
            tempLeft = 0x7F;
        }
    } else {
        tempLeft = 0;
    }

    left = tempLeft;
    if (right >= 0) {
        tempRight = right;
        if (tempRight >= 0x80) {
            tempRight = 0x7F;
        }
    } else {
        tempRight = 0;
    }
    right = tempRight;

    SsUtReverbOff();

    presetIndex = type - 1;
    if (presetIndex < 9) {
        g_ReverbType = type;
        g_ReverbDepthL = left;
        g_ReverbDepthR = right;
        SsUtSetReverbType((s16)type);
        SsUtReverbOn();
        SetReverbDepth(left, right);
    } else {
        g_ReverbType = 0;
        g_ReverbDepthR = 0;
        g_ReverbDepthL = 0;
    }
}

void PlaySoundSlotVoice(s32 slot, s32 tone, s32 vabSlot) {
    s16 program = g_SoundSlotTone[slot][tone];

    SsUtKeyOnV((s16)(slot + 0xE), g_SoundScale.vabIds[(s16)vabSlot], program, 0, 0x3C, 0, 0, 0);
}
