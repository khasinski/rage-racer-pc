#include "game/race.h"
#include "game/render.h"
#include "game/track_internal.h"

enum {
    ENVIRONMENT_PALETTE_COLOR_COUNT = 16,
    ENVIRONMENT_FOG_NEAR = 0x1770,
    ENVIRONMENT_FOG_FAR = 0x7FFF,
    ENVIRONMENT_FOG_STEP = 0xFA,
};

/* The two-word header is followed immediately by GameEnvironmentCue records. */
void SetEnvironmentScript(u32 *script) {
    g_SkyRowBase = *script++;
    g_EnvScriptLength = *script++;
    g_EnvScriptCues = (GameEnvironmentCue *)(void *)script;
}

static GameEnvironmentCue *LastEnvironmentCue(void) {
    GameEnvironmentCue *cue = g_EnvScriptCues;

    while (cue[1].time != -1) {
        cue++;
    }
    return cue;
}

static GameEnvironmentCue *NextEnvironmentCue(GameEnvironmentCue *cue) {
    return cue[1].time < 0 ? g_EnvScriptCues : cue + 1;
}

static GameEnvironmentCue *PreviousCueAtClock(s32 clock) {
    GameEnvironmentCue *cue = g_EnvScriptCues;
    s32 cueCount = 0;

    while (cue[cueCount].time != -1 && cue[cueCount].time <= clock) {
        cueCount++;
    }
    if (cueCount < 2) {
        return LastEnvironmentCue();
    }
    return cue + cueCount - 2;
}

static void SetCurrentEnvironmentColors(const GameEnvironmentCue *cue) {
    s32 slot;

    for (slot = 0; slot < ENV_SLOT_COUNT; slot++) {
        g_EnvironmentColors.fields.slots[slot].cur = cue->colors[slot];
    }
    g_EnvironmentMode = cue->mode;
}

static void ApplyFogSettings(void) {
    GameEnvColor fog = g_EnvironmentColors.fields.slots[ENV_FOG].cur;
    s32 fogEnabled = 0;

    if ((g_EnvironmentColors.fogColorWord & 0xFFFF0000) != 0x80800000 ||
        fog.bytes.b != 0x80) {
        fogEnabled = 1;
    }
    g_EnvironmentColors.fields.fogEnabled = (s16)fogEnabled;
    SetFarColor(fog.bytes.r, fog.bytes.g, fog.bytes.b);

    g_FogNear = g_EnvironmentMode == 2
        ? ENVIRONMENT_FOG_FAR
        : ENVIRONMENT_FOG_NEAR;
    SetFogNear(g_FogNear, 0x140);
}

void SeekEnvironmentScript(s32 targetTime) {
    GameEnvironmentCue *previousCue;
    GameEnvironmentCue *targetCue;
    s32 frame;

    g_EnvScriptClock = (targetTime + g_EnvScriptLength) % g_EnvScriptLength;
    previousCue = PreviousCueAtClock(g_EnvScriptClock);
    SetCurrentEnvironmentColors(previousCue);

    targetCue = NextEnvironmentCue(previousCue);
    LoadEnvironmentCue(targetCue);
    frame = (u16)g_EnvScriptClock - (u16)targetCue->time;
    g_EnvLerpFrame = (s16)frame > g_EnvLerpDuration
        ? g_EnvLerpDuration
        : (s16)frame;
    g_EnvScriptCursor = NextEnvironmentCue(targetCue);

    g_EnvScriptEnabled = 1;
    g_EnvironmentColors.fields.fogEnabled = 1;
    UpdateEnvironment();
    if (g_GrandPrixClass >= 5) {
        g_EnvScriptEnabled = 0;
    }
    ApplyFogSettings();
}

static u16 InterpolateClutColor(const Rgb *from, const Rgb *to, s32 blend) {
    u16 red = (u16)LerpColorChannel(from->r, to->r, blend);
    u16 green = (u16)LerpColorChannel(from->g, to->g, blend);
    u16 blue = (u16)LerpColorChannel(from->b, to->b, blend);

    return (u16)(red | (green << 5) | (blue << 10));
}

static void UpdateEnvironmentPalette(s32 blend) {
    Rect rect = {0xE0, 0x1E6, 0x10, 1};
    s32 color;

    for (color = 0; color < ENVIRONMENT_PALETTE_COLOR_COUNT; color++) {
        const Rgb *from = &g_EnvPaletteTable[g_EnvironmentModePrev].colors[color];
        const Rgb *to = &g_EnvPaletteTable[g_EnvironmentMode].colors[color];

        g_EnvironmentClut[color] = InterpolateClutColor(from, to, blend);
    }
    LoadImage(&rect, (u_long *)g_EnvironmentClut);
}

static void UpdateEnvironmentColorSlots(s32 blend) {
    s32 slot;

    for (slot = ENV_FOG; slot <= ENV_SKY_BOTTOM; slot++) {
        LerpEnvColor(&g_EnvironmentColors.fields.slots[slot].from,
                     &g_EnvironmentColors.fields.slots[slot].to,
                     &g_EnvironmentColors.fields.slots[slot].cur, blend);
    }

    if (g_CourseIndex == 2) {
        for (slot = ENV_GROUND_NEAR_TOP;
             slot <= ENV_GROUND_NEAR_BOTTOM; slot++) {
            LerpEnvColor(&g_EnvironmentColors.fields.slots[slot].from,
                         &g_EnvironmentColors.fields.slots[slot].to,
                         &g_EnvironmentColors.fields.slots[slot].cur, blend);
        }
    } else {
        for (slot = ENV_GROUND_FAR_TOP;
             slot <= ENV_GROUND_FAR_BOTTOM; slot++) {
            LerpEnvColor(&g_EnvironmentColors.fields.slots[slot].from,
                         &g_EnvironmentColors.fields.slots[slot].to,
                         &g_EnvironmentColors.fields.slots[slot].cur, blend);
        }
    }
}

static void UpdateFogDistance(void) {
    if (g_EnvironmentMode == 2) {
        g_FogNear += ENVIRONMENT_FOG_STEP;
        if (g_FogNear > ENVIRONMENT_FOG_FAR) {
            g_FogNear = ENVIRONMENT_FOG_FAR;
        }
    } else {
        g_FogNear -= ENVIRONMENT_FOG_STEP;
        if (g_FogNear < ENVIRONMENT_FOG_NEAR) {
            g_FogNear = ENVIRONMENT_FOG_NEAR;
        }
    }
    SetFogNear(g_FogNear, 0x140);
}

void UpdateEnvironment(void) {
    GameEnvColor fog;
    s32 remainingFrames;
    s32 blend;

    if (g_EnvScriptEnabled == 0) {
        return;
    }

    if (g_EnvScriptCursor->time == g_EnvScriptClock) {
        GameEnvironmentCue *cue = g_EnvScriptCursor;

        g_EnvLerpFrame = 0;
        g_EnvScriptCursor = NextEnvironmentCue(cue);
        LoadEnvironmentCue(cue);
    }

    g_EnvScriptClock = g_EnvScriptClock < g_EnvScriptLength
        ? g_EnvScriptClock + 1
        : 0;
    if (g_EnvironmentColors.fields.fogEnabled == 0) {
        return;
    }
    if (g_EnvLerpFrame < g_EnvLerpDuration) {
        g_EnvLerpFrame++;
    }

    remainingFrames = g_EnvLerpDuration - g_EnvLerpFrame;
    blend = (g_EnvLerpFrame << 12) / g_EnvLerpDuration;
    UpdateEnvironmentPalette(blend);
    UpdateEnvironmentColorSlots(blend);

    fog = g_EnvironmentColors.fields.slots[ENV_FOG].cur;
    SetFarColor(fog.bytes.r, fog.bytes.g, fog.bytes.b);
    if (g_EnvSpareLerp != 0) {
        g_EnvironmentColors.fields.slots[ENV_FOG].cur.bytes.unused =
            (u8)((g_EnvSpareFrom * remainingFrames +
                  g_EnvSpareTo * g_EnvLerpFrame) / g_EnvLerpDuration);
    }

    if (g_EnvLerpFrame == g_EnvLerpDuration &&
        (g_EnvironmentColors.fogColorWord & 0xFFFF0000) == 0x80800000 &&
        fog.bytes.b == 0x80) {
        g_EnvironmentColors.fields.fogEnabled = 0;
    }
    UpdateFogDistance();
}
