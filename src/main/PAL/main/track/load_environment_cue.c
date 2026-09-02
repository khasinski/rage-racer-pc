#include "game/render.h"
#include "game/track.h"

static s16 EnvironmentCueDuration(u16 duration) {
    if (duration == 0) {
        return 1;
    }
    return duration > 0x7FFF ? 0x7FFF : (s16)duration;
}

void LoadEnvironmentCue(GameEnvironmentCue *cue) {
    s32 i;
    s32 mode;
    u32 flag;

    g_EnvironmentColors.fields.fogEnabled = 1;

    for (i = 0; i < 9; i++) {
        g_EnvironmentColors.fields.slots[i].to = cue->colors[i];
        g_EnvironmentColors.fields.slots[i].from =
            g_EnvironmentColors.fields.slots[i].cur;
    }

    mode = g_EnvironmentMode;
    /* A zero-duration authored cue is an instantaneous transition. Treat it
     * as one update so all interpolation paths reach their target without
     * dividing by zero. */
    g_EnvLerpDuration = EnvironmentCueDuration(cue->duration);
    g_EnvironmentMode = cue->mode;
    flag = cue->spareTarget;
    g_EnvironmentModePrev = mode;
    g_EnvSpareLerp = ((flag >> 15) ^ 1);

    if (g_EnvSpareLerp != 0) {
        g_EnvSpareFrom = g_EnvironmentColors.fields.slots[ENV_FOG].cur.bytes.unused;
        g_EnvSpareTo = cue->spareTarget;
    }

    g_IsEnvironmentMode4 = (s16)g_EnvironmentMode == 4;
}
