#include "game/race.h"
#include "game/render.h"
#include "game/track_internal.h"

enum {
    ENVIRONMENT_PALETTE_COLOR_COUNT = 16,
    ENVIRONMENT_CLUT_X = 0xE0,
    ENVIRONMENT_CLUT_Y = 0x1E6,
    ENVIRONMENT_FOG_NEAR = 0x1770,
    ENVIRONMENT_FOG_FAR = 0x7FFF,
    ENVIRONMENT_FOG_STEP = 0xFA,
    ENVIRONMENT_FAR_FOG_MODE = 2,
};

static s16 EnvironmentCueDuration(u16 duration) {
    if (duration == 0) return 1;
    return duration > INT16_MAX ? INT16_MAX : (s16)duration;
}

static void LerpEnvironmentColor(const GameEnvColor *from,
                                 const GameEnvColor *to,
                                 GameEnvColor *out, s32 blend) {
    out->bytes.r = LerpColorChannel(from->bytes.r, to->bytes.r, blend);
    out->bytes.g = LerpColorChannel(from->bytes.g, to->bytes.g, blend);
    out->bytes.b = LerpColorChannel(from->bytes.b, to->bytes.b, blend);
}

static void LoadEnvironmentCue(const GameEnvironmentCue *cue) {
    s32 slot;
    s16 previousMode = g_EnvironmentMode;

    g_EnvironmentColors.fields.fogEnabled = 1;
    for (slot = 0; slot < ENV_SLOT_COUNT; slot++) {
        GameEnvColorSlot *color =
            &g_EnvironmentColors.fields.slots[slot];

        color->from = color->cur;
        color->to = cue->colors[slot];
    }

    /* A zero-duration cue is instantaneous. One update reaches its target
     * without introducing a division-by-zero special case downstream. */
    g_EnvLerpDuration = EnvironmentCueDuration(cue->duration);
    g_EnvironmentModePrev = previousMode;
    g_EnvironmentMode = (s16)cue->mode;
    g_EnvSpareLerp = (cue->spareTarget & 0x8000) == 0;
    if (g_EnvSpareLerp != 0) {
        g_EnvSpareFrom =
            g_EnvironmentColors.fields.slots[ENV_FOG].cur.bytes.unused;
        g_EnvSpareTo = (s16)cue->spareTarget;
    }
    g_IsEnvironmentMode4 = g_EnvironmentMode == 4;
}

static void ClearEnvironmentScript(void) {
    g_SkyRowBase = 0;
    g_EnvScriptLength = 0;
    g_EnvScriptCues = NULL;
}

s32 IsValidEnvironmentScript(const GameEnvironmentScript *script,
                             size_t size) {
    size_t cueCount;
    size_t i;
    s32 previousTime = -1;

    if (script == NULL || size < offsetof(GameEnvironmentScript, cues) ||
        script->skyRowBase > SKY_TILE_MAP_ROWS - 2 ||
        script->length == 0 || script->length > INT32_MAX) {
        return 0;
    }
    cueCount = (size - offsetof(GameEnvironmentScript, cues)) /
               sizeof(script->cues[0]);
    if (cueCount < 2 || script->cues[0].time != 0) return 0;

    for (i = 0; i < cueCount; i++) {
        const GameEnvironmentCue *cue = &script->cues[i];

        if (cue->time == -1) return i != 0;
        if (cue->time < 0 || (u32)cue->time >= script->length ||
            cue->time <= previousTime ||
            cue->mode >= ENVIRONMENT_PALETTE_COUNT) {
            return 0;
        }
        previousTime = cue->time;
    }
    return 0;
}

s32 SetEnvironmentScript(const GameEnvironmentScript *script, size_t size) {
    if (!IsValidEnvironmentScript(script, size)) {
        ClearEnvironmentScript();
        return 0;
    }

    g_SkyRowBase = script->skyRowBase;
    g_EnvScriptLength = (s32)script->length;
    g_EnvScriptCues = script->cues;
    return 1;
}

static const GameEnvironmentCue *LastEnvironmentCue(void) {
    const GameEnvironmentCue *cue = g_EnvScriptCues;

    while (cue[1].time != -1) {
        cue++;
    }
    return cue;
}

static const GameEnvironmentCue *NextEnvironmentCue(
    const GameEnvironmentCue *cue) {
    return cue[1].time < 0 ? g_EnvScriptCues : cue + 1;
}

static const GameEnvironmentCue *PreviousCueAtClock(s32 clock) {
    const GameEnvironmentCue *cue = g_EnvScriptCues;
    s32 cueCount = 0;

    while (cue[cueCount].time != -1 && cue[cueCount].time <= clock) {
        cueCount++;
    }
    if (cueCount < 2) {
        return LastEnvironmentCue();
    }
    return cue + cueCount - 2;
}

static s32 NormalizeEnvironmentTime(s32 time, s32 length) {
    time %= length;
    return time < 0 ? time + length : time;
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

    g_FogNear = g_EnvironmentMode == ENVIRONMENT_FAR_FOG_MODE
        ? ENVIRONMENT_FOG_FAR
        : ENVIRONMENT_FOG_NEAR;
    SetFogNear(g_FogNear, SCREEN_WIDTH);
}

void SeekEnvironmentScript(s32 targetTime) {
    const GameEnvironmentCue *previousCue;
    const GameEnvironmentCue *targetCue;
    s32 frame;

    if (g_EnvScriptLength <= 0 || g_EnvScriptCues == NULL) {
        g_EnvScriptClock = 0;
        g_EnvScriptEnabled = 0;
        return;
    }

    g_EnvScriptClock =
        NormalizeEnvironmentTime(targetTime, g_EnvScriptLength);
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
    if (g_GrandPrixClass >= GRAND_PRIX_FINAL_CLASS_INDEX) {
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
    Rect rect = {
        ENVIRONMENT_CLUT_X,
        ENVIRONMENT_CLUT_Y,
        ENVIRONMENT_PALETTE_COLOR_COUNT,
        1,
    };
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
    s32 firstGroundSlot;
    s32 lastGroundSlot;

    for (slot = ENV_FOG; slot <= ENV_SKY_BOTTOM; slot++) {
        LerpEnvironmentColor(&g_EnvironmentColors.fields.slots[slot].from,
                             &g_EnvironmentColors.fields.slots[slot].to,
                             &g_EnvironmentColors.fields.slots[slot].cur,
                             blend);
    }

    if (g_CourseIndex == 2) {
        firstGroundSlot = ENV_GROUND_NEAR_TOP;
        lastGroundSlot = ENV_GROUND_NEAR_BOTTOM;
    } else {
        firstGroundSlot = ENV_GROUND_FAR_TOP;
        lastGroundSlot = ENV_GROUND_FAR_BOTTOM;
    }
    for (slot = firstGroundSlot; slot <= lastGroundSlot; slot++) {
        LerpEnvironmentColor(
            &g_EnvironmentColors.fields.slots[slot].from,
            &g_EnvironmentColors.fields.slots[slot].to,
            &g_EnvironmentColors.fields.slots[slot].cur, blend);
    }
}

static void UpdateFogDistance(void) {
    if (g_EnvironmentMode == ENVIRONMENT_FAR_FOG_MODE) {
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
    SetFogNear(g_FogNear, SCREEN_WIDTH);
}

void UpdateEnvironment(void) {
    GameEnvColor fog;
    s32 remainingFrames;
    s32 blend;

    if (g_EnvScriptEnabled == 0) {
        return;
    }
    if (g_EnvLerpDuration <= 0) {
        g_EnvLerpDuration = 1;
    }

    if (g_EnvScriptCursor->time == g_EnvScriptClock) {
        const GameEnvironmentCue *cue = g_EnvScriptCursor;

        g_EnvLerpFrame = 0;
        g_EnvScriptCursor = NextEnvironmentCue(cue);
        LoadEnvironmentCue(cue);
    }

    g_EnvScriptClock = g_EnvScriptClock < g_EnvScriptLength - 1
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
