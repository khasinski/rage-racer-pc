#include "game/audio.h"
#include "game/audio_internal.h"
#include "game/sound.h"

enum { DEFAULT_ENGINE_MAX_RPM = 0x2800 };

static u16 ReadTableValue(const u8 **cursor) {
    const u8 *bytes = *cursor;

    *cursor += sizeof(u16);
    return (u16)(bytes[0] | (u16)bytes[1] << 8);
}

void LoadAudioParameterTable(const void *data, size_t size) {
    const u8 *cursor = data;
    s32 bank;
    s32 row;
    s32 column;
    s32 maxRpm;

    if (data == NULL || size < ENGINE_SOUND_PARAMETER_TABLE_SIZE) {
        return;
    }

    for (bank = 0; bank < ENGINE_SOUND_BANK_COUNT; bank++) {
        for (row = 0; row < ENGINE_SOUND_PARAMETER_COUNT; row++) {
            for (column = 0; column < ENGINE_SOUND_CURVE_POINT_COUNT;
                 column++) {
                g_EngineSoundCurves[bank][row].positions[column] =
                    ReadTableValue(&cursor);
                g_EngineSoundCurves[bank][row].values[column] =
                    ReadTableValue(&cursor);
            }
        }
    }

    SetLoadedTableVolumeScale(ReadTableValue(&cursor));
    for (bank = 0; bank < ENGINE_SOUND_BANK_COUNT; bank++) {
        for (row = 0; row < ENGINE_SOUND_SLOT_COUNT; row++) {
            g_SoundSlotTone[row][bank] = (s16)ReadTableValue(&cursor);
        }
    }

    maxRpm = ReadTableValue(&cursor);
    g_EngineSoundState.maxRpm =
        maxRpm == 0 || maxRpm >= DEFAULT_ENGINE_MAX_RPM
            ? DEFAULT_ENGINE_MAX_RPM
            : maxRpm;
}
