#include "game/environment.h"
#include "game/track.h"
#include <stdint.h>

s32 IsValidEnvironmentScript(const GameEnvironmentScript *script,
                             size_t size) {
    size_t cueCount;
    size_t i;
    s32 previousTime = -1;

    if (script == NULL || ((uintptr_t)script & 3u) != 0 || size < offsetof(GameEnvironmentScript, cues) ||
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

s32 EnvironmentTime(s32 time, s32 length) {
    if (length <= 0) return 0;
    time %= length;
    return time < 0 ? time + length : time;
}

s16 EnvironmentCueFrame(s32 clock, s32 cueTime, s32 loopLength,
                               s16 duration) {
    if (clock < 0 || clock >= loopLength || cueTime < 0 ||
        cueTime >= loopLength || duration <= 0) return 0;
    int64_t elapsed = (int64_t)clock - cueTime;

    if (elapsed < 0) {
        elapsed += loopLength;
    }
    return elapsed > duration ? duration : (s16)elapsed;
}

/* Source must be a validated, terminated cue sequence; clock is normalized. */
const struct GameEnvironmentCue *EnvironmentCueAt(const struct GameEnvironmentCue *cues, s32 clock) {
    if (!cues || clock < 0) return NULL;
    while (cues[1].time >= 0 && cues[1].time <= clock) ++cues;
    return cues;
}
static void LerpEnvironmentColor(const GameEnvColor *from,
                                 const GameEnvColor *to,
                                 GameEnvColor *out, s32 blend) {
    out->bytes.r = LerpColorChannel(from->bytes.r, to->bytes.r, blend);
    out->bytes.g = LerpColorChannel(from->bytes.g, to->bytes.g, blend);
    out->bytes.b = LerpColorChannel(from->bytes.b, to->bytes.b, blend);
}

static u16 InterpolateClutColor(const Rgb *from, const Rgb *to, s32 blend) {
    u16 red = (u16)LerpColorChannel(from->r, to->r, blend);
    u16 green = (u16)LerpColorChannel(from->g, to->g, blend);
    u16 blue = (u16)LerpColorChannel(from->b, to->b, blend);

    return (u16)(red | (green << 5) | (blue << 10));
}

int BlendEnvironmentColors(GameEnvironmentColors *colors, s32 course, s32 blend) {
    if (!colors || blend < 0 || blend > 4096) return 0;
    s32 slot;
    s32 firstGroundSlot;
    s32 lastGroundSlot;

    for (slot = ENV_FOG; slot <= ENV_SKY_BOTTOM; slot++) {
        LerpEnvironmentColor(&colors->fields.slots[slot].from,
                             &colors->fields.slots[slot].to,
                             &colors->fields.slots[slot].cur,
                             blend);
    }

    if (course == 2) {
        firstGroundSlot = ENV_GROUND_NEAR_TOP;
        lastGroundSlot = ENV_GROUND_NEAR_BOTTOM;
    } else {
        firstGroundSlot = ENV_GROUND_FAR_TOP;
        lastGroundSlot = ENV_GROUND_FAR_BOTTOM;
    }
    for (slot = firstGroundSlot; slot <= lastGroundSlot; slot++) {
        LerpEnvironmentColor(
            &colors->fields.slots[slot].from,
            &colors->fields.slots[slot].to,
            &colors->fields.slots[slot].cur, blend);
    }
    return 1;
}

int BlendEnvironmentPalette(const EnvironmentPalette *from, const EnvironmentPalette *to,
                             s32 blend, u16 output[16]) {
    if (!from || !to || !output || blend < 0 || blend > 4096) return 0;
    for (s32 color = 0; color < 16; ++color)
        output[color] = InterpolateClutColor(&from->colors[color], &to->colors[color], blend);
    return 1;
}
