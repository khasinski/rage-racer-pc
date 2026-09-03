#include "common.h"
#include "game/audio.h"
#include "game/audio_internal.h"
#include "game/sound.h"

#include <stdio.h>
#include <string.h>

EngineSoundCurveRow
    g_EngineSoundCurves[ENGINE_SOUND_BANK_COUNT][ENGINE_SOUND_PARAMETER_COUNT];
EngineSoundState g_EngineSoundState;
s16 g_SoundSlotTone[ENGINE_SOUND_SLOT_COUNT][ENGINE_SOUND_BANK_COUNT];

static s32 s_volumeScale;
static s32 s_failures;

void SetLoadedTableVolumeScale(s32 scale) { s_volumeScale = scale; }

static void Check(s32 actual, s32 expected, const char *label) {
    if (actual != expected) {
        printf("FAIL %s: got %d, expected %d\n", label, actual, expected);
        s_failures++;
    }
}

static void WriteLittleEndianU16(u8 *destination, u16 value) {
    destination[0] = (u8)value;
    destination[1] = (u8)(value >> 8);
}

static void LoadWithMaxRpm(u16 maxRpm) {
    u8 storage[ENGINE_SOUND_PARAMETER_TABLE_SIZE + 1];
    u8 *table = storage + 1;
    s32 i;

    for (i = 0; i < ENGINE_SOUND_PARAMETER_TABLE_WORD_COUNT; i++) {
        WriteLittleEndianU16(table + i * sizeof(u16), (u16)(100 + i));
    }
    WriteLittleEndianU16(
        table + (ENGINE_SOUND_PARAMETER_TABLE_WORD_COUNT - 1) * sizeof(u16),
        maxRpm);
    LoadAudioParameterTable(table, ENGINE_SOUND_PARAMETER_TABLE_SIZE);
}

int main(void) {
    u8 truncated[ENGINE_SOUND_PARAMETER_TABLE_SIZE - 1] = {0};

    g_EngineSoundState.maxRpm = 123;
    LoadAudioParameterTable(NULL, 0);
    LoadAudioParameterTable(truncated, sizeof(truncated));
    Check(g_EngineSoundState.maxRpm, 123, "truncated table ignored");
    memset(g_SoundSlotTone, 0, sizeof(g_SoundSlotTone));
    LoadWithMaxRpm(9000);
    Check(g_EngineSoundCurves[0][0].positions[0], 100,
          "first curve position");
    Check(g_EngineSoundCurves[0][0].values[0], 101,
          "first curve value");
    Check(g_EngineSoundCurves[1][11].positions[8], 530,
          "last curve position");
    Check(g_EngineSoundCurves[1][11].values[8], 531,
          "last curve value");
    Check(s_volumeScale, 532, "volume scale");
    Check(g_SoundSlotTone[0][0], 533, "first tone");
    Check(g_SoundSlotTone[5][0], 538, "last first-bank tone");
    Check(g_SoundSlotTone[0][1], 539, "first second-bank tone");
    Check(g_SoundSlotTone[5][1], 544, "last tone");
    Check(g_EngineSoundState.maxRpm, 9000, "valid maximum rpm");

    LoadWithMaxRpm(0);
    Check(g_EngineSoundState.maxRpm, 0x2800, "zero maximum rpm fallback");
    LoadWithMaxRpm(0x27FF);
    Check(g_EngineSoundState.maxRpm, 0x27FF, "largest valid maximum rpm");
    LoadWithMaxRpm(0x2800);
    Check(g_EngineSoundState.maxRpm, 0x2800, "maximum rpm upper fallback");
    LoadWithMaxRpm(0xFFFF);
    Check(g_EngineSoundState.maxRpm, 0x2800, "oversized maximum rpm fallback");

    if (s_failures != 0) return 1;
    puts("audio parameter tables map curves, tones and rpm limits");
    return 0;
}
