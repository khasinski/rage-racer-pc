#include "game/car.h"
#include "game/track_internal.h"

enum {
    TACHOMETER_DARK_ZONE = 3,
    DAWN_FADE_START = 0x1154,
    DAYLIGHT_START = 0x11D4,
    DUSK_FADE_START = 0x5420,
    NIGHT_START = 0x54A0,
};

void DrawPlayerTachometer(void) {
    TachometerLightingMode lighting;
    s32 blendAmount;

    if (g_TrackZoneDark == TACHOMETER_DARK_ZONE ||
        g_EnvScriptClock < DAWN_FADE_START ||
        g_EnvScriptClock >= NIGHT_START) {
        lighting = TACHOMETER_LIGHTING_DARK;
        blendAmount = 0;
    } else if (g_EnvScriptClock < DAYLIGHT_START) {
        lighting = TACHOMETER_LIGHTING_FADE_FROM_DARK;
        blendAmount = g_EnvScriptClock - DAWN_FADE_START;
    } else if (g_EnvScriptClock < DUSK_FADE_START) {
        lighting = TACHOMETER_LIGHTING_NORMAL;
        blendAmount = 0;
    } else {
        lighting = TACHOMETER_LIGHTING_FADE_TO_DARK;
        blendAmount = g_EnvScriptClock - DUSK_FADE_START;
    }

    DrawTachometer(g_EngineRpm + g_EngineRpmJitter, g_TachoShiftLightOn,
                   lighting, blendAmount);
}
