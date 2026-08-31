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

/* Reads one tone out of the 6x2 g_SoundSlotTone grid, and writes it too when
 * `tone` is not negative. Returns what was there before. */
s32 SetSoundToneTableEntry(s32 slot, s32 vabSlot, s32 tone) {
    s16 *entry = &g_SoundSlotTone[slot][vabSlot];
    s32 old;

    old = *entry;

    if (tone >= 0) {
        *entry = tone;
    }
    return old;
}

void LoadAudioParameterTable(u16 *table) {
    u16 *tableReg = table;
    s32 bank;
    s32 row;
    s32 col;
    s32 step;
    s32 tableValue;

    for (bank = 0; bank < 2; bank++) {
        for (row = 0; row < 12; row++) {
            for (col = 0; col < 9; col++) {
                g_EngineSoundCurves[bank][row].positions[col] = *tableReg++;
                g_EngineSoundCurves[bank][row].values[col] = *tableReg++;
            }
        }
    }

    tableValue = *tableReg++;
    SetLoadedTableVolumeScale(tableValue);

    for (bank = 0; bank < 2; bank++) {
        for (row = 0; row < 6; row++) {
            SetSoundToneTableEntry(row, bank, *tableReg++);
        }
    }

    step = *tableReg;
    g_EngineSoundState.maxRpm = step;
    if ((u32)(step - 1) >= 0x27FF) {
        g_EngineSoundState.maxRpm = 0x2800;
    }
}

void SetReverbDepth(s32 left, s32 right) {
    left = ClampCueLevel(left);
    right = ClampCueLevel(right);

    g_ReverbDepthL = left;
    g_ReverbDepthR = right;
    SsUtSetReverbDepth((s16)left, (s16)right);
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
