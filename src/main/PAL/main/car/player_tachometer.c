#include "game/car.h"
#include "game/track_internal.h"

void DrawPlayerTachometer(void) {
    s32 type;
    u32 blendAmount;

    if (g_TrackZoneDark == 3 || g_EnvScriptClock < 0x1154 ||
        g_EnvScriptClock >= 0x54A0) {
        type = 2;
        blendAmount = 0;
    } else if (g_EnvScriptClock < 0x11D4) {
        type = 3;
        blendAmount = (u32)(g_EnvScriptClock - 0x1154);
    } else if (g_EnvScriptClock < 0x5420) {
        type = 0;
        blendAmount = 0;
    } else {
        type = 1;
        blendAmount = (u32)(g_EnvScriptClock - 0x5420);
    }

    DrawTachometer(g_EngineRpm + g_EngineRpmJitter, g_TachoNeedleFlash,
                   type, blendAmount);
}
