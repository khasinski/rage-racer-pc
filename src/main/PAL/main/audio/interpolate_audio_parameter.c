#include "game/audio.h"
#include "game/sound.h"

enum {
    LAST_ENGINE_SOUND_CURVE_POINT = ENGINE_SOUND_CURVE_POINT_COUNT - 1,
};

s32 InterpolateAudioParameter(s32 parameter, s32 position, s32 bank) {
    const EngineSoundCurveRow *curve;
    s32 upperPoint = 1;
    s32 positionSpan;
    s32 value;

    if ((u32)bank >= ENGINE_SOUND_BANK_COUNT ||
        (u32)parameter >= ENGINE_SOUND_PARAMETER_COUNT) {
        return 0;
    }
    curve = &g_EngineSoundCurves[bank][parameter];

    while (upperPoint < ENGINE_SOUND_CURVE_POINT_COUNT &&
           position >= curve->positions[upperPoint]) {
        upperPoint++;
    }
    if (upperPoint == ENGINE_SOUND_CURVE_POINT_COUNT) {
        /* Retail returns the final stored value verbatim; only interpolated
         * values are cue-level clamped. */
        return curve->values[LAST_ENGINE_SOUND_CURVE_POINT];
    }

    positionSpan = curve->positions[upperPoint] -
                   curve->positions[upperPoint - 1];
    if (positionSpan <= 0) {
        return ClampCueLevel(curve->values[upperPoint]);
    }

    value = (curve->values[upperPoint] - curve->values[upperPoint - 1]) *
                (position - curve->positions[upperPoint - 1]) / positionSpan +
            curve->values[upperPoint - 1];
    return ClampCueLevel(value);
}
