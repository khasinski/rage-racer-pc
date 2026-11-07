#include "game/environment.h"

enum {
    ENVIRONMENT_FOG_NEAR = 0x1770,
    ENVIRONMENT_FOG_FAR = 0x7FFF,
    ENVIRONMENT_FOG_STEP = 0xFA,
    ENVIRONMENT_FAR_FOG_MODE = 2,
};

static s16 EnvironmentCueDuration(u16 duration) {
    if (duration == 0) return 1;
    return duration > INT16_MAX ? INT16_MAX : (s16)duration;
}

static void LoadEnvironmentCue(Environment *env, const struct GameEnvironmentCue *cue) {
    s32 slot;
    s16 previousMode = env->mode;

    env->colors.fields.fogEnabled = 1;
    for (slot = 0; slot < ENV_SLOT_COUNT; slot++) {
        GameEnvColorSlot *color =
            &env->colors.fields.slots[slot];

        color->from = color->cur;
        color->to = cue->colors[slot];
    }

    /* A zero-duration cue is instantaneous. One update reaches its target
     * without introducing a division-by-zero special case downstream. */
    env->duration = EnvironmentCueDuration(cue->duration);
    env->previousMode = previousMode;
    env->mode = (s16)cue->mode;
    env->spareLerp = (cue->spareTarget & 0x8000) == 0;
    if (env->spareLerp != 0) {
        env->spareFrom =
            env->colors.fields.slots[ENV_FOG].cur.bytes.unused;
        env->spareTo = (s16)cue->spareTarget;
    }
    env->mode4 = env->mode == 4;
}

static void UpdateFogDistance(Environment *env) {
    if (env->mode == ENVIRONMENT_FAR_FOG_MODE) {
        if (env->fogNear >= ENVIRONMENT_FOG_FAR - ENVIRONMENT_FOG_STEP) {
            env->fogNear = ENVIRONMENT_FOG_FAR;
        } else {
            env->fogNear += ENVIRONMENT_FOG_STEP;
        }
    } else {
        if (env->fogNear <= ENVIRONMENT_FOG_NEAR + ENVIRONMENT_FOG_STEP) {
            env->fogNear = ENVIRONMENT_FOG_NEAR;
        } else {
            env->fogNear -= ENVIRONMENT_FOG_STEP;
        }
    }
}

int TickEnvironment(Environment *env) {
    if (!env || !env->palettes || !env->cues || !env->next || env->length <= 0) return 0;
    if (env->clock < 0 || env->clock >= env->length || env->mode < 0 ||
        env->mode >= ENVIRONMENT_PALETTE_COUNT || env->previousMode < 0 ||
        env->previousMode >= ENVIRONMENT_PALETTE_COUNT ||
        env->next->mode >= ENVIRONMENT_PALETTE_COUNT) return 0;
    GameEnvColor fog;
    s32 remainingFrames;
    s32 blend;

    if (env->enabled == 0) {
        return 0;
    }
    if (env->duration <= 0) {
        env->duration = 1;
    }

    if (env->next->time == env->clock) {
        const struct GameEnvironmentCue *cue = env->next;

        env->frame = 0;
        env->next = cue[1].time < 0 ? env->cues : cue + 1;
        LoadEnvironmentCue(env, cue);
    }

    env->clock = env->clock < env->length - 1
                           ? env->clock + 1
                           : 0;
    if (env->colors.fields.fogEnabled == 0) {
        return 0;
    }
    if (env->frame < env->duration) {
        env->frame++;
    }

    remainingFrames = env->duration - env->frame;
    blend = (env->frame << 12) / env->duration;
    BlendEnvironmentPalette(&env->palettes[env->previousMode],
                            &env->palettes[env->mode], blend, env->clut);
    BlendEnvironmentColors(&env->colors, env->course, blend);

    fog = env->colors.fields.slots[ENV_FOG].cur;
    if (env->spareLerp != 0) {
        env->colors.fields.slots[ENV_FOG].cur.bytes.unused =
            (u8)((env->spareFrom * remainingFrames +
                  env->spareTo * env->frame) / env->duration);
    }

    if (env->frame == env->duration &&
        (env->colors.fogColorWord & 0xFFFF0000) == 0x80800000 &&
        fog.bytes.b == 0x80) {
        env->colors.fields.fogEnabled = 0;
    }
    UpdateFogDistance(env);
    return 1;
}

void SeekEnvironment(Environment *env, s32 time) {
    if (!env || !env->cues || !env->palettes || env->length <= 0) return;
    env->clock = EnvironmentTime(time, env->length);
    const struct GameEnvironmentCue *target = EnvironmentCueAt(env->cues, env->clock);
    const struct GameEnvironmentCue *previous = target == env->cues
        ? EnvironmentCueAt(env->cues, env->length - 1) : target - 1;
    for (s32 slot = 0; slot < ENV_SLOT_COUNT; ++slot)
        env->colors.fields.slots[slot].cur = previous->colors[slot];
    env->mode = previous->mode;
    LoadEnvironmentCue(env, target);
    env->frame = EnvironmentCueFrame(env->clock, target->time, env->length, env->duration);
    env->next = target[1].time < 0 ? env->cues : target + 1;
    env->enabled = 1;
    TickEnvironment(env);
    GameEnvColor fog = env->colors.fields.slots[ENV_FOG].cur;
    env->colors.fields.fogEnabled =
        (env->colors.fogColorWord & 0xffff0000u) != 0x80800000u || fog.bytes.b != 0x80;
    env->fogNear = env->mode == ENVIRONMENT_FAR_FOG_MODE ? ENVIRONMENT_FOG_FAR : ENVIRONMENT_FOG_NEAR;
}

int InitEnvironment(Environment *env, const GameEnvironmentScript *script, size_t size,
                     const EnvironmentPalette *palettes, s32 course) {
    if (!env || !palettes || !IsValidEnvironmentScript(script, size)) return 0;
    Environment candidate = {.cues = script->cues, .palettes = palettes,
        .length = (s32)script->length, .skyRowBase = (s32)script->skyRowBase, .course = course};
    SeekEnvironment(&candidate, 0);
    *env = candidate;
    return 1;
}
