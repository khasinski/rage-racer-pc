#include "game/car.h"
#include "game/track_internal.h"

#include <limits.h>
#include <stdio.h>

s32 g_EngineRpm;
s32 g_EngineRpmJitter;
s32 g_TachoShiftLightOn;
s32 g_EnvScriptClock;
s16 g_TrackZoneDark;

static s32 s_rpm;
static s32 s_flash;
static TachometerLightingMode s_lighting;
static s32 s_amount;
static int s_calls;
static int s_failures;

void DrawTachometer(s32 rpm, s32 flash, TachometerLightingMode lighting,
                    s32 amount) {
    s_rpm = rpm;
    s_flash = flash;
    s_lighting = lighting;
    s_amount = amount;
    s_calls++;
}

static void CheckClock(s32 clock, s32 dark,
                       TachometerLightingMode expectedLighting,
                       s32 expectedAmount) {
    g_EnvScriptClock = clock;
    g_TrackZoneDark = (s16)dark;
    s_calls = 0;
    DrawPlayerTachometer();
    if (s_calls != 1 || s_rpm != 5123 || s_flash != 1 ||
        s_lighting != expectedLighting || s_amount != expectedAmount) {
        printf("FAIL clock=%d dark=%d: calls=%d rpm=%d flash=%d "
               "type=%d amount=%d; expected type=%d amount=%d\n",
               clock, dark, s_calls, s_rpm, s_flash, s_lighting, s_amount,
               expectedLighting, expectedAmount);
        s_failures++;
    }
}

int main(void) {
    g_EngineRpm = 5000;
    g_EngineRpmJitter = 123;
    g_TachoShiftLightOn = 1;

    CheckClock(0x1153, 0, TACHOMETER_LIGHTING_DARK, 0);
    CheckClock(0x1154, 0, TACHOMETER_LIGHTING_FADE_FROM_DARK, 0);
    CheckClock(0x11D3, 0, TACHOMETER_LIGHTING_FADE_FROM_DARK, 0x7F);
    CheckClock(0x11D4, 0, TACHOMETER_LIGHTING_NORMAL, 0);
    CheckClock(0x541F, 0, TACHOMETER_LIGHTING_NORMAL, 0);
    CheckClock(0x5420, 0, TACHOMETER_LIGHTING_FADE_TO_DARK, 0);
    CheckClock(0x549F, 0, TACHOMETER_LIGHTING_FADE_TO_DARK, 0x7F);
    CheckClock(0x54A0, 0, TACHOMETER_LIGHTING_DARK, 0);
    CheckClock(0x1154, 3, TACHOMETER_LIGHTING_DARK, 0);

    g_EngineRpm = INT_MAX;
    g_EngineRpmJitter = 1;
    DrawPlayerTachometer();
    if (s_rpm != INT_MIN) {
        printf("FAIL extreme displayed RPM became %d\n", s_rpm);
        s_failures++;
    }

    if (s_failures != 0) {
        printf("%d player tachometer checks failed\n", s_failures);
        return 1;
    }
    puts("player tachometer palette windows are bounded");
    return 0;
}
