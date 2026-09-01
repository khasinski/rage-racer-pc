#include "game/audio.h"
#include "game/sound.h"

enum {
    ENGINE_SOUND_CURVE_POINT_COUNT = 9,
    AUDIO_PARAMETER_MAX = 0x7F,
};

s32 InterpolateAudioParameter(s32 parameter, s32 position, s32 bank) {
    const EngineSoundCurveRow *curve =
        &g_EngineSoundCurves[bank][parameter];
    s32 upperPoint = 1;
    s32 value;

    while (upperPoint < ENGINE_SOUND_CURVE_POINT_COUNT &&
           position >= curve->positions[upperPoint]) {
        upperPoint++;
    }
    if (upperPoint == ENGINE_SOUND_CURVE_POINT_COUNT) {
        return curve->values[ENGINE_SOUND_CURVE_POINT_COUNT - 1];
    }

    value = (curve->values[upperPoint] - curve->values[upperPoint - 1]) *
                (position - curve->positions[upperPoint - 1]) /
                (curve->positions[upperPoint] -
                 curve->positions[upperPoint - 1]) +
            curve->values[upperPoint - 1];
    if (value < 0) {
        return 0;
    }
    if (value > AUDIO_PARAMETER_MAX) {
        return AUDIO_PARAMETER_MAX;
    }
    return value;
}
