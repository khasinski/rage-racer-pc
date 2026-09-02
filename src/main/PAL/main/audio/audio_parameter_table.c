#include "game/audio.h"
#include "game/sound.h"

enum { DEFAULT_ENGINE_MAX_RPM = 0x2800 };

void LoadAudioParameterTable(const u16 *table) {
    const u16 *cursor = table;
    s32 bank;
    s32 row;
    s32 column;
    s32 maxRpm;

    for (bank = 0; bank < ENGINE_SOUND_BANK_COUNT; bank++) {
        for (row = 0; row < ENGINE_SOUND_PARAMETER_COUNT; row++) {
            for (column = 0; column < ENGINE_SOUND_CURVE_POINT_COUNT;
                 column++) {
                g_EngineSoundCurves[bank][row].positions[column] = *cursor++;
                g_EngineSoundCurves[bank][row].values[column] = *cursor++;
            }
        }
    }

    SetLoadedTableVolumeScale(*cursor++);
    for (bank = 0; bank < ENGINE_SOUND_BANK_COUNT; bank++) {
        for (row = 0; row < ENGINE_SOUND_SLOT_COUNT; row++) {
            g_SoundSlotTone[row][bank] = (s16)*cursor++;
        }
    }

    maxRpm = *cursor;
    g_EngineSoundState.maxRpm =
        maxRpm == 0 || maxRpm >= DEFAULT_ENGINE_MAX_RPM
            ? DEFAULT_ENGINE_MAX_RPM
            : maxRpm;
}
