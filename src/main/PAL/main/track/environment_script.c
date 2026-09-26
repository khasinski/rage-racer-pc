#include "game/race.h"
#include "game/render.h"
#include "game/track_internal.h"
#include <string.h>

static void ClearEnvironmentScript(void) {
    g_SkyRowBase = 0;
    g_EnvScriptLength = 0;
    g_EnvScriptClock = 0;
    g_EnvScriptCues = NULL;
    g_EnvScriptCursor = NULL;
    g_EnvScriptEnabled = 0;
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

static Environment LegacyEnvironment(void) {
    Environment env = {
        .colors = g_EnvironmentColors,
        .duration = g_EnvLerpDuration,
        .previousMode = g_EnvironmentModePrev,
        .mode = g_EnvironmentMode,
        .spareLerp = g_EnvSpareLerp,
        .spareFrom = g_EnvSpareFrom,
        .spareTo = g_EnvSpareTo,
        .mode4 = g_IsEnvironmentMode4,
        .fogNear = g_FogNear,
        .enabled = g_EnvScriptEnabled,
        .clock = g_EnvScriptClock,
        .length = g_EnvScriptLength,
        .next = g_EnvScriptCursor,
        .frame = g_EnvLerpFrame,
        .cues = g_EnvScriptCues,
        .skyRowBase = g_SkyRowBase,
        .palettes = g_EnvPaletteTable, .course = g_CourseIndex,
    };
    memcpy(env.clut, g_EnvironmentClut, sizeof(env.clut));
    return env;
}

static void StoreEnvironment(const Environment *env) {
    g_EnvironmentColors = env->colors;
    g_EnvLerpDuration = env->duration;
    g_EnvironmentModePrev = env->previousMode;
    g_EnvironmentMode = env->mode;
    g_EnvSpareLerp = env->spareLerp;
    g_EnvSpareFrom = env->spareFrom;
    g_EnvSpareTo = env->spareTo;
    g_IsEnvironmentMode4 = env->mode4;
    g_FogNear = env->fogNear;
    g_EnvScriptEnabled = env->enabled;
    g_EnvScriptClock = env->clock;
    g_EnvScriptLength = env->length;
    g_EnvScriptCursor = env->next;
    g_EnvLerpFrame = env->frame;
    g_EnvScriptCues = env->cues;
    g_SkyRowBase = env->skyRowBase;
    memcpy(g_EnvironmentClut, env->clut, sizeof(env->clut));
}

static void PresentEnvironment(const Environment *env) {
    Rect rect = {0xE0, 0x1E6, 16, 1};
    LoadImage(&rect, (u_long *)g_EnvironmentClut);
    GameEnvColor fog = env->colors.fields.slots[ENV_FOG].cur;
    SetFarColor(fog.bytes.r, fog.bytes.g, fog.bytes.b);
    SetFogNear(env->fogNear, SCREEN_WIDTH);
}

void SeekEnvironmentScript(s32 targetTime) {
    if (g_EnvScriptLength <= 0 || !g_EnvScriptCues) {
        g_EnvScriptClock = 0;
        g_EnvScriptEnabled = 0;
        return;
    }
    Environment env = LegacyEnvironment();
    SeekEnvironment(&env, targetTime);
    if (g_GrandPrixClass >= GRAND_PRIX_FINAL_CLASS_INDEX) env.enabled = 0;
    StoreEnvironment(&env);
    PresentEnvironment(&env);
}

void UpdateEnvironment(void) {
    Environment env = LegacyEnvironment();
    int changed = TickEnvironment(&env);
    StoreEnvironment(&env);
    if (changed) PresentEnvironment(&env);
}
