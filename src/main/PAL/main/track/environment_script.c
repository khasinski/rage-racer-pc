#include "game/race.h"
#include "game/render.h"

#include "game/track_internal.h"

void SeekEnvironmentScript(s32 targetTime) {
    s32 clock;
    s32 count;
    s32 tailCount;
    s32 nextId;
    s32 duration;
    s32 frame;
    s32 clampedFrame;
    s32 signedFrame;
    s32 fog;
    GameEnvironmentCue *cue;
    EnvironmentScriptLocation scriptLocation;

    scriptLocation.time = targetTime;
    clock = (scriptLocation.time + g_EnvScriptLength) % g_EnvScriptLength;
    scriptLocation.pointer = g_EnvScriptCues;
    g_EnvScriptCursor = scriptLocation.pointer;
    g_EnvScriptClock = clock;
    for (count = 0;
         scriptLocation.pointer[count].time != -1;
         count++) {
        if (clock < scriptLocation.pointer[count].time) {
            break;
        }
    }

    if (count >= 2) {
        g_EnvScriptCursor += count - 2;
    } else {
        scriptLocation.pointer = g_EnvScriptCursor;
        for (tailCount = 0;
             scriptLocation.pointer[tailCount + 1].time != -1;
             tailCount++) {
        }
        g_EnvScriptCursor += tailCount;
    }

    for (count = 0; count < 9; count++)
        g_EnvironmentColors.fields.slots[count].cur =
            g_EnvScriptCursor->colors[count];

    g_EnvironmentMode = g_EnvScriptCursor->mode;
    nextId = g_EnvScriptCursor[1].time;
    g_EnvScriptCursor++;
    if (nextId < 0) {
        g_EnvScriptCursor = g_EnvScriptCues;
    }

    cue = g_EnvScriptCursor;
    duration = cue->duration;
    g_EnvLerpDuration = duration;
    frame = (u16)g_EnvScriptClock - (u16)cue->time;
    g_EnvLerpFrame = frame;
    clampedFrame = frame;
    /* Keep the unclamped store and the call-value copy as distinct lifetimes. */
    
    signedFrame = (s16)frame;
    if ((s16)duration < signedFrame) {
        clampedFrame = duration;
    }
    g_EnvLerpFrame = clampedFrame;
    /* The cue carries its own duration, and the frame goes through
     * g_EnvLerpFrame set just above; the two extra arguments here were
     * never read. */
    LoadEnvironmentCue(cue);

    nextId = g_EnvScriptCursor[1].time;
    g_EnvScriptCursor++;
    if (nextId < 0) {
        g_EnvScriptCursor = g_EnvScriptCues;
    }

    g_EnvScriptEnabled = 1;
    g_EnvironmentColors.fields.fogEnabled = 1;
    UpdateEnvironment();

    fog = 0;
    if (g_GrandPrixClass >= 5) {
        g_EnvScriptEnabled = 0;
    }
    if ((g_EnvironmentColors.fogColorWord & 0xFFFF0000) != 0x80800000 ||
        g_EnvironmentColors.fields.slots[ENV_FOG].cur.bytes.b != 0x80) {
        fog = 1;
    }
    g_EnvironmentColors.fields.fogEnabled = fog;
    SetFarColor(g_EnvironmentColors.fields.slots[ENV_FOG].cur.bytes.r,
                g_EnvironmentColors.fields.slots[ENV_FOG].cur.bytes.g,
                g_EnvironmentColors.fields.slots[ENV_FOG].cur.bytes.b);

    if (g_EnvironmentMode == 2) {
        g_FogNear = 0x7FFF;
    } else {
        g_FogNear = 0x1770;
    }
    SetFogNear(g_FogNear, 0x140);
}

void UpdateEnvironment(void) {
    Rect rect;
    u8 out[3];
    s32 i;
    s32 diff;
    s32 frac;
    GameEnvironmentCue *cur;

    if (g_EnvScriptEnabled == 0) {
        return;
    }

    cur = g_EnvScriptCursor;
    if (cur->time == g_EnvScriptClock) {
        g_EnvLerpFrame = 0;
        g_EnvScriptCursor = cur + 1;
        LoadEnvironmentCue(cur);
        if (g_EnvScriptCursor->time < 0) {
            g_EnvScriptCursor = g_EnvScriptCues;
        }
    }

    g_EnvScriptClock = (g_EnvScriptClock < g_EnvScriptLength) ? g_EnvScriptClock + 1 : 0;

    if (g_EnvironmentColors.fields.fogEnabled == 0) {
        return;
    }

    if (g_EnvScriptEnabled != 0) {
        if (g_EnvLerpFrame < g_EnvLerpDuration) {
            g_EnvLerpFrame++;
        }
    }

    diff = g_EnvLerpDuration - g_EnvLerpFrame;
    frac = (g_EnvLerpFrame << 12) / g_EnvLerpDuration;

    for (i = 0; i < 0x10; i++) {
        Rgb *p1 = &g_EnvPaletteTable[g_EnvironmentModePrev].colors[i];
        Rgb *p2 = &g_EnvPaletteTable[g_EnvironmentMode].colors[i];
        s16 *dst;

        out[0] = LerpColorChannel(p1->r, p2->r, frac);
        out[1] = LerpColorChannel(p1->g, p2->g, frac);
        out[2] = LerpColorChannel(p1->b, p2->b, frac);
        dst = (s16 *)&g_EnvironmentClut[i];
        *dst = out[0];
        *dst |= out[1] << 5;
        *dst |= out[2] << 10;
    }

    rect.x = 0xE0;
    rect.y = 0x1E6;
    rect.w = 0x10;
    rect.h = 0x1;
    LoadImage(&rect, (u_long *)g_EnvironmentClut);

    for (i = 0; i < 5; i++)
        LerpEnvColor(&g_EnvironmentColors.fields.slots[i].from,
                     &g_EnvironmentColors.fields.slots[i].to,
                     &g_EnvironmentColors.fields.slots[i].cur, frac);
    if (g_CourseIndex == 2) {
        for (i = 5; i < 7; i++)
            LerpEnvColor(&g_EnvironmentColors.fields.slots[i].from,
                         &g_EnvironmentColors.fields.slots[i].to,
                         &g_EnvironmentColors.fields.slots[i].cur, frac);
    } else {
        for (i = 7; i < 9; i++)
            LerpEnvColor(&g_EnvironmentColors.fields.slots[i].from,
                         &g_EnvironmentColors.fields.slots[i].to,
                         &g_EnvironmentColors.fields.slots[i].cur, frac);
    }

    SetFarColor(g_EnvironmentColors.fields.slots[ENV_FOG].cur.bytes.r,
                g_EnvironmentColors.fields.slots[ENV_FOG].cur.bytes.g,
                g_EnvironmentColors.fields.slots[ENV_FOG].cur.bytes.b);

    if (g_EnvSpareLerp != 0) {
        g_EnvironmentColors.fields.slots[ENV_FOG].cur.bytes.unused = (g_EnvSpareFrom * diff + g_EnvSpareTo * g_EnvLerpFrame) / g_EnvLerpDuration;
    }

    if (g_EnvLerpFrame == g_EnvLerpDuration) {
        if ((g_EnvironmentColors.fogColorWord & 0xFFFF0000) == 0x80800000 &&
            g_EnvironmentColors.fields.slots[ENV_FOG].cur.bytes.b == 0x80) {
            g_EnvironmentColors.fields.fogEnabled = 0;
        }
    }

    if (g_EnvironmentMode == 2) {
        g_FogNear += 0xFA;
        if (g_FogNear > 0x7FFF) {
            g_FogNear = 0x7FFF;
        }
    } else {
        g_FogNear -= 0xFA;
        if (g_FogNear < 0x1770) {
            g_FogNear = 0x1770;
        }
    }

    SetFogNear(g_FogNear, 0x140);
}
