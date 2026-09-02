#include "common.h"
#include "game/audio.h"

#include <stdio.h>
#include <string.h>

EngineSoundCurveRow
    g_EngineSoundCurves[ENGINE_SOUND_BANK_COUNT][ENGINE_SOUND_PARAMETER_COUNT];

#define CHECK(condition) do {                                                  \
    if (!(condition)) {                                                        \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition);\
        return 1;                                                              \
    }                                                                          \
} while (0)

int main(void) {
    EngineSoundCurveRow *curve = &g_EngineSoundCurves[1][3];
    s32 point;

    memset(g_EngineSoundCurves, 0, sizeof(g_EngineSoundCurves));
    for (point = 0; point < ENGINE_SOUND_CURVE_POINT_COUNT; point++) {
        curve->positions[point] = point * 100;
        curve->values[point] = point * 20;
    }

    CHECK(InterpolateAudioParameter(3, 150, 1) == 30);
    CHECK(InterpolateAudioParameter(3, 50, 1) == 10);
    CHECK(InterpolateAudioParameter(3, 750, 1) == 0x7F);
    CHECK(InterpolateAudioParameter(3, 800, 1) == 160);
    CHECK(InterpolateAudioParameter(3, -50, 1) == 0);

    curve->positions[2] = curve->positions[1];
    curve->values[2] = 50;
    CHECK(InterpolateAudioParameter(3, 100, 1) == 50);

    curve->values[0] = -20;
    curve->values[1] = 0;
    CHECK(InterpolateAudioParameter(3, 25, 1) == 0);

    puts("audio parameter interpolation preserves segments and clamping");
    return 0;
}
