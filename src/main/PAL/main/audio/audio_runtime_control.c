#include "game/audio.h"
#include "game/sound.h"
#include "game/state.h"
#include "game/work_buffer.h"
#include "psyq/snd.h"

void TickSequenceAudio(void) {
    if (g_SceneId == 0xC) {
        SpuVmDamperStep();
    } else {
        /* The host owns the sequence clock. Menu frames already run at the
         * selected PAL/NTSC base rate, so one service call is one audio tick. */
        SsSeqCalledTbyT();
        if (g_SeqVolumeFadeStep != 0) {
            UpdateSequenceFadeOut();
        }
        /* libsnd batches voice register writes. On PS1 its sound interrupt
         * flushed the final key-off; the host drives this service directly. */
        SpuVmDamperStep();
    }
}


s32 IsSpuTransferDone(void) {
    SpuTransferSampleBuffer *buffer;
    s32 value0;
    s32 value1;

    buffer = &g_ReplayFrameBuffer.spuTransfer;
    value1 = SpuTransferStatus(buffer, 0);
    value0 = buffer->channelA[value1][0];
    value1 = buffer->channelB[value1][0];

    value0 = value0 < 0 ? -value0 : value0;
    value1 = value1 < 0 ? -value1 : value1;

    return (value0 << 16) | (s16)value1;
}

/* Reads one tone out of the 6x2 g_SoundSlotTone grid, and writes it too when
 * `tone` is not negative. Returns what was there before. */
s32 SetSoundToneTableEntry(s32 slot, s32 vabSlot, s32 tone) {
    s16 (*table)[2];
    s16 *row;
    s16 *entry;
    s32 old;
    SoundToneTableAddress entryAddress;

    table = g_SoundSlotTone;
    row = table[slot];
    entryAddress.pointer = row;
    entryAddress.value = vabSlot * sizeof(*entry) + entryAddress.value;
    entry = entryAddress.pointer;
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
    s32 *leftPtr;
    s32 tableValue;
    u32 adjustedStep;

    bank = 0;
    do {
        row = 0;
        do {
            col = 0;
            do {
                s32 leftValue;

                leftValue = *tableReg++;
                g_EngineSoundCurves[bank][row].positions[col] = leftValue;
                g_EngineSoundCurves[bank][row].values[col] = *tableReg++;
                col++;
            } while (col < 9);
            row++;
        } while (row < 12);
        bank++;
    } while (bank < 2);

    tableValue = *tableReg;
    tableReg++;
    bank = 0;
    SetLoadedTableVolumeScale(tableValue);

    do {
        row = 0;
        do {
            s32 rowArg;

            tableValue = *tableReg;
            tableReg++;
            rowArg = row;
            row++;
            SetSoundToneTableEntry(rowArg, bank, tableValue);
        } while (row < 6);
        bank++;
    } while (bank < 2);

    step = *tableReg;
    leftPtr = &g_EngineSoundState.maxRpm;
    *leftPtr = step;
    step--;
    adjustedStep = step;
    if (adjustedStep >= 0x27FF) {
        *leftPtr = 0x2800;
    }
}

void SetReverbDepth(s32 left, s32 right) {
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

    if ((left = tempLeft, right) >= 0) {
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
    s16 (*table)[2];
    s16 *row;
    s16 *entry;
    SoundToneTableAddress entryAddress;

    table = g_SoundSlotTone;
    row = table[slot];
    entryAddress.pointer = row;
    entryAddress.value = tone * sizeof(*entry) + entryAddress.value;
    entry = entryAddress.pointer;
    SsUtKeyOnV((s16)(slot + 0xE), g_SoundScale.vabIds[(s16)vabSlot], *entry, 0, 0x3C, 0, 0, 0);
}
