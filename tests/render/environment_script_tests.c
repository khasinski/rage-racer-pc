#include "common.h"
#include "game/asset.h"
#include "game/race.h"
#include "game/render.h"
#include "game/track_internal.h"

#include <stdio.h>
#include <string.h>

GameEnvironmentCue *g_EnvScriptCues;
GameEnvironmentCue *g_EnvScriptCursor;
s32 g_EnvScriptLength;
s32 g_EnvScriptClock;
u8 g_EnvScriptEnabled;
GameEnvironmentColors g_EnvironmentColors;
s16 g_EnvLerpFrame;
s16 g_EnvLerpDuration;
s16 g_EnvironmentMode;
s32 g_EnvironmentModePrev;
s16 g_EnvSpareLerp;
s16 g_EnvSpareFrom;
s16 g_EnvSpareTo;
s32 g_IsEnvironmentMode4;
s32 g_GrandPrixClass;
s32 g_CourseIndex;
s32 g_FogNear;
s32 g_SkyRowBase;
EnvironmentPalette *g_EnvPaletteTable;
u16 g_EnvironmentClut[16];

static s32 g_LoadImageCalls;
static s32 g_FarRed;
static s32 g_FarGreen;
static s32 g_FarBlue;
static s32 g_FogNearCall;

#undef LoadImage
int LoadImage(RECT *rect, u_long *data) {
    (void)rect;
    (void)data;
    g_LoadImageCalls++;
    return 0;
}

void SetFarColor(long red, long green, long blue) {
    g_FarRed = (s32)red;
    g_FarGreen = (s32)green;
    g_FarBlue = (s32)blue;
}

void SetFogNear(long nearValue, long projection) {
    (void)projection;
    g_FogNearCall = (s32)nearValue;
}

static GameEnvColor Color(u8 value) {
    GameEnvColor color = {{0}};

    color.bytes.r = value;
    color.bytes.g = value;
    color.bytes.b = value;
    return color;
}

static s32 CheckColorInterpolation(void) {
    GameEnvColor from = Color(10);
    GameEnvColor to = Color(250);
    GameEnvColor out = Color(0);

    LerpEnvColor(&from, &to, &out, 0);
    if (out.bytes.r != 10 || out.bytes.g != 10 || out.bytes.b != 10) {
        return 0;
    }
    LerpEnvColor(&from, &to, &out, 0x1000);
    if (out.bytes.r != 250 || out.bytes.g != 250 || out.bytes.b != 250) {
        return 0;
    }
    LerpEnvColor(&to, &from, &out, 0x800);
    return out.bytes.r == 130 && out.bytes.g == 130 && out.bytes.b == 130;
}

static void SeedCue(GameEnvironmentCue *cue, s32 time, u8 color,
                    u16 duration, u16 mode) {
    s32 slot;

    memset(cue, 0, sizeof(*cue));
    cue->time = time;
    cue->duration = duration;
    cue->mode = mode;
    cue->spareTarget = 0x8000;
    for (slot = 0; slot < ENV_SLOT_COUNT; slot++) {
        cue->colors[slot] = Color(color);
    }
}

int main(void) {
    GameEnvironmentScript script;
    GameEnvironmentCue cues[4];
    EnvironmentPalette palettes[5];
    s32 color;

    if (!CheckColorInterpolation()) {
        puts("FAIL: environment color interpolation");
        return 1;
    }

    script.skyRowBase = 7;
    script.length = 123;
    SetEnvironmentScript(&script);
    if (g_SkyRowBase != 7 || g_EnvScriptLength != 123 ||
        g_EnvScriptCues != script.cues) {
        puts("FAIL: environment script header");
        return 1;
    }

    SeedCue(&cues[0], 0, 4, 10, 0);
    SeedCue(&cues[1], 10, 12, 10, 2);
    SeedCue(&cues[2], 20, 20, 10, 4);
    memset(&cues[3], 0, sizeof(cues[3]));
    cues[3].time = -1;
    memset(palettes, 0, sizeof(palettes));
    for (color = 0; color < 16; color++) {
        palettes[0].colors[color] = (Rgb){0, 0, 0};
        palettes[2].colors[color] = (Rgb){31, 15, 7};
        palettes[4].colors[color] = (Rgb){7, 15, 31};
    }
    g_EnvScriptCues = cues;
    g_EnvPaletteTable = palettes;
    g_EnvScriptLength = 30;
    g_GrandPrixClass = 0;
    g_CourseIndex = 0;

    memset(&g_EnvironmentColors, 0, sizeof(g_EnvironmentColors));
    g_LoadImageCalls = 0;
    SeekEnvironmentScript(15);
    if (g_EnvScriptClock != 16 || g_EnvScriptCursor != &cues[2] ||
        g_EnvLerpDuration != 10 || g_EnvLerpFrame != 6 ||
        g_EnvironmentModePrev != 0 || g_EnvironmentMode != 2 ||
        g_EnvironmentColors.fields.slots[ENV_FOG].from.bytes.r != 4 ||
        g_EnvironmentColors.fields.slots[ENV_FOG].to.bytes.r != 12 ||
        g_EnvironmentColors.fields.slots[ENV_FOG].cur.bytes.r != 8 ||
        g_LoadImageCalls != 1 || g_EnvironmentClut[0] != 4370 ||
        g_FogNear != 0x7FFF ||
        g_FogNearCall != 0x7FFF ||
        g_FarRed != 8 || g_FarGreen != 8 || g_FarBlue != 8) {
        puts("FAIL: seek inside environment cue");
        return 1;
    }

    SeekEnvironmentScript(5);
    if (g_EnvScriptClock != 6 || g_EnvScriptCursor != &cues[1] ||
        g_EnvironmentModePrev != 4 || g_EnvironmentMode != 0 ||
        g_EnvironmentColors.fields.slots[ENV_FOG].from.bytes.r != 20 ||
        g_EnvironmentColors.fields.slots[ENV_FOG].to.bytes.r != 4) {
        puts("FAIL: wrapped environment seek");
        return 1;
    }

    cues[1].duration = 0;
    SeekEnvironmentScript(15);
    if (g_EnvLerpDuration != 1 || g_EnvLerpFrame != 1 ||
        g_EnvironmentColors.fields.slots[ENV_FOG].cur.bytes.r != 12) {
        puts("FAIL: zero-duration environment cue");
        return 1;
    }
    cues[1].duration = 0xFFFF;
    LoadEnvironmentCue(&cues[1]);
    if (g_EnvLerpDuration != 0x7FFF) {
        puts("FAIL: oversized environment cue duration");
        return 1;
    }
    cues[1].duration = 10;

    g_EnvScriptEnabled = 1;
    g_EnvironmentColors.fields.fogEnabled = 0;
    g_EnvScriptClock = 29;
    UpdateEnvironment();
    if (g_EnvScriptClock != 30) {
        puts("FAIL: environment clock reaches script length");
        return 1;
    }
    UpdateEnvironment();
    if (g_EnvScriptClock != 0) {
        puts("FAIL: environment clock wraps after script length");
        return 1;
    }

    g_GrandPrixClass = 5;
    SeekEnvironmentScript(15);
    if (g_EnvScriptEnabled != 0) {
        puts("FAIL: final class environment animation disabled");
        return 1;
    }

    puts("environment script behavior preserved");
    return 0;
}
