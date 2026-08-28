#include "game/render.h"
#include "game/track.h"

void LoadEnvironmentCue(GameEnvironmentCue *cue) {
    s32 mode;
    s32 newMode;
    s32 compareMode;
    s32 signedMode;
    s32 field28;
    u32 flag;

    g_EnvironmentColors.fields.fogEnabled = 1;

    g_EnvironmentColors.fields.slots[0].to = cue->colors[0];
    g_EnvironmentColors.fields.slots[0].from = g_EnvironmentColors.fields.slots[0].cur;
    g_EnvironmentColors.fields.slots[1].to = cue->colors[1];
    g_EnvironmentColors.fields.slots[1].from = g_EnvironmentColors.fields.slots[1].cur;
    g_EnvironmentColors.fields.slots[2].to = cue->colors[2];
    g_EnvironmentColors.fields.slots[2].from = g_EnvironmentColors.fields.slots[2].cur;
    g_EnvironmentColors.fields.slots[3].to = cue->colors[3];
    g_EnvironmentColors.fields.slots[3].from = g_EnvironmentColors.fields.slots[3].cur;
    g_EnvironmentColors.fields.slots[4].to = cue->colors[4];
    g_EnvironmentColors.fields.slots[4].from = g_EnvironmentColors.fields.slots[4].cur;
    g_EnvironmentColors.fields.slots[5].to = cue->colors[5];
    g_EnvironmentColors.fields.slots[5].from = g_EnvironmentColors.fields.slots[5].cur;
    g_EnvironmentColors.fields.slots[6].to = cue->colors[6];
    g_EnvironmentColors.fields.slots[6].from = g_EnvironmentColors.fields.slots[6].cur;
    g_EnvironmentColors.fields.slots[7].to = cue->colors[7];
    g_EnvironmentColors.fields.slots[7].from = g_EnvironmentColors.fields.slots[7].cur;
    g_EnvironmentColors.fields.slots[8].to = cue->colors[8];
    g_EnvironmentColors.fields.slots[8].from = g_EnvironmentColors.fields.slots[8].cur;

    field28 = cue->duration;
    mode = g_EnvironmentMode;
    g_EnvLerpDuration = field28;
    newMode = RAW(cue->mode);
    g_EnvironmentMode = newMode;
    flag = RAW(cue->spareTarget);
    g_EnvironmentModePrev = mode;
    g_EnvSpareLerp = ((flag >> 15) ^ 1);
    compareMode = 4;

    if (g_EnvSpareLerp != 0) {
        g_EnvSpareFrom = g_EnvironmentColors.fields.slots[0].cur.bytes.unused;
        g_EnvSpareTo = RAW(cue->spareTarget);
    }

    signedMode = (s16)newMode;
    if (signedMode == compareMode) {
        g_IsEnvironmentMode4 = 1;
    } else {
        g_IsEnvironmentMode4 = 0;
    }
}
