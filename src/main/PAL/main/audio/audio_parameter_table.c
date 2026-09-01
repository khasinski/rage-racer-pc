#include "game/audio.h"
#include "game/sound.h"

/* Reads one tone out of the 6x2 g_SoundSlotTone grid, and writes it too when
 * `tone` is not negative. Returns what was there before. */
s32 SetSoundToneTableEntry(s32 slot, s32 vabSlot, s32 tone) {
    s16 *entry = &g_SoundSlotTone[slot][vabSlot];
    s32 previous = *entry;

    if (tone >= 0) {
        *entry = (s16)tone;
    }
    return previous;
}

void LoadAudioParameterTable(u16 *table) {
    u16 *cursor = table;
    s32 bank;
    s32 row;
    s32 column;
    s32 maxRpm;

    for (bank = 0; bank < 2; bank++) {
        for (row = 0; row < 12; row++) {
            for (column = 0; column < 9; column++) {
                g_EngineSoundCurves[bank][row].positions[column] = *cursor++;
                g_EngineSoundCurves[bank][row].values[column] = *cursor++;
            }
        }
    }

    SetLoadedTableVolumeScale(*cursor++);
    for (bank = 0; bank < 2; bank++) {
        for (row = 0; row < 6; row++) {
            SetSoundToneTableEntry(row, bank, *cursor++);
        }
    }

    maxRpm = *cursor;
    g_EngineSoundState.maxRpm =
        maxRpm == 0 || maxRpm >= 0x2800 ? 0x2800 : maxRpm;
}
