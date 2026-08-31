#include "game/render.h"
#include "game/track.h"

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
    g_EnvLerpDuration = cue->duration;
    g_EnvironmentMode = RAW(cue->mode);
    flag = RAW(cue->spareTarget);
    g_EnvironmentModePrev = mode;
    g_EnvSpareLerp = ((flag >> 15) ^ 1);

    if (g_EnvSpareLerp != 0) {
        g_EnvSpareFrom = g_EnvironmentColors.fields.slots[0].cur.bytes.unused;
        g_EnvSpareTo = RAW(cue->spareTarget);
    }

    g_IsEnvironmentMode4 = (s16)g_EnvironmentMode == 4;
}
