#include "common.h"
#include "psyq/gpu.h"
#include "game/race.h"
#include "game/render.h"
#include "game/sound.h"
#include "game/track.h"

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
    s16 *fogOut;
    s16 *fogTarget;

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

    g_EnvironmentColors.fields.slots[0].cur = g_EnvScriptCursor->colors[0];
    g_EnvironmentColors.fields.slots[1].cur = g_EnvScriptCursor->colors[1];
    g_EnvironmentColors.fields.slots[2].cur = g_EnvScriptCursor->colors[2];
    g_EnvironmentColors.fields.slots[3].cur = g_EnvScriptCursor->colors[3];
    g_EnvironmentColors.fields.slots[4].cur = g_EnvScriptCursor->colors[4];
    g_EnvironmentColors.fields.slots[5].cur = g_EnvScriptCursor->colors[5];
    g_EnvironmentColors.fields.slots[6].cur = g_EnvScriptCursor->colors[6];
    g_EnvironmentColors.fields.slots[7].cur = g_EnvScriptCursor->colors[7];
    g_EnvironmentColors.fields.slots[8].cur = g_EnvScriptCursor->colors[8];

    g_EnvironmentMode = g_EnvScriptCursor->mode;
    nextId = RAW(g_EnvScriptCursor[1].time);
    g_EnvScriptCursor = g_EnvScriptCursor + 1;
    if (nextId < 0) {
        g_EnvScriptCursor = g_EnvScriptCues;
    }

    cue = g_EnvScriptCursor;
    duration = cue->duration;
    g_EnvLerpDuration = duration;
    frame = (u16)g_EnvScriptClock - (u16)RAW(cue->time);
    g_EnvLerpFrame = frame;
    clampedFrame = frame;
    /* Keep the unclamped store and the call-value copy as distinct lifetimes. */
    asm("" : "=r"(clampedFrame) : "0"(clampedFrame));
    signedFrame = (s16)frame;
    if ((s16)duration < signedFrame) {
        clampedFrame = duration;
    }
    g_EnvLerpFrame = clampedFrame;
    LoadEnvironmentCue(cue, duration, clampedFrame);

    nextId = g_EnvScriptCursor[1].time;
    g_EnvScriptCursor = g_EnvScriptCursor + 1;
    if (nextId < 0) {
        g_EnvScriptCursor = g_EnvScriptCues;
    }

    fogOut = &g_EnvironmentColors.fields.fogEnabled;
    g_EnvScriptEnabled = 1;
    *fogOut = 1;
    UpdateEnvironment();

    fog = 0;
    if (g_GrandPrixClass >= 5) {
        g_EnvScriptEnabled = 0;
    }
    fogTarget = fogOut;
    if ((g_EnvironmentColors.fogColorWord & 0xFFFF0000) != 0x80800000 ||
        g_EnvironmentColors.fields.slots[0].cur.bytes.b != 0x80) {
        fog = 1;
    }
    *fogTarget = fog;
    SetFarColor(g_EnvironmentColors.fields.slots[0].cur.bytes.r,
                g_EnvironmentColors.fields.slots[0].cur.bytes.g,
                g_EnvironmentColors.fields.slots[0].cur.bytes.b);

    if (g_EnvironmentMode == 2) {
        g_FogNear = 0x7FFF;
    } else {
        g_FogNear = 0x1770;
    }
    SetFogNear(g_FogNear, 0x140);
}

void UpdateEnvironment(void) {
    Rect rect;
    u8 out[4];
    s32 i;
    s32 diff;
    s32 frac;
    Rgb *p1;
    Rgb *p2;
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
            g_EnvLerpFrame = g_EnvLerpFrame + 1;
        }
    }

    diff = g_EnvLerpDuration - g_EnvLerpFrame;
    frac = (g_EnvLerpFrame << 12) / g_EnvLerpDuration;

    for (i = 0; i < 0x10; i++) {
        s16 *dst;
        p1 = &g_EnvPaletteTable[g_EnvironmentModePrev].colors[i];
        p2 = &g_EnvPaletteTable[g_EnvironmentMode].colors[i];
        out[0] = LerpColorChannel(p1->r, p2->r, frac);
        out[1] = LerpColorChannel(p1->g, p2->g, frac);
        out[2] = LerpColorChannel(p1->b, p2->b, frac);
        dst = (s16 *)&g_EnvironmentClut[i];
        *dst = 0;
        *dst = out[0];
        *dst |= out[1] << 5;
        *dst |= out[2] << 10;
    }

    rect.x = 0xE0;
    rect.y = 0x1E6;
    rect.w = 0x10;
    rect.h = 0x1;
    LoadImage(&rect, (u_long *)g_EnvironmentClut);

    LerpEnvColor(&g_EnvironmentColors.fields.slots[0].from, &g_EnvironmentColors.fields.slots[0].to,
                 &g_EnvironmentColors.fields.slots[0].cur, frac);
    LerpEnvColor(&g_EnvironmentColors.fields.slots[1].from, &g_EnvironmentColors.fields.slots[1].to,
                 &g_EnvironmentColors.fields.slots[1].cur, frac);
    LerpEnvColor(&g_EnvironmentColors.fields.slots[2].from, &g_EnvironmentColors.fields.slots[2].to,
                 &g_EnvironmentColors.fields.slots[2].cur, frac);
    LerpEnvColor(&g_EnvironmentColors.fields.slots[3].from, &g_EnvironmentColors.fields.slots[3].to,
                 &g_EnvironmentColors.fields.slots[3].cur, frac);
    LerpEnvColor(&g_EnvironmentColors.fields.slots[4].from, &g_EnvironmentColors.fields.slots[4].to,
                 &g_EnvironmentColors.fields.slots[4].cur, frac);
    if (g_CourseIndex == 2) {
        LerpEnvColor(&g_EnvironmentColors.fields.slots[5].from, &g_EnvironmentColors.fields.slots[5].to,
                     &g_EnvironmentColors.fields.slots[5].cur, frac);
        LerpEnvColor(&g_EnvironmentColors.fields.slots[6].from, &g_EnvironmentColors.fields.slots[6].to,
                     &g_EnvironmentColors.fields.slots[6].cur, frac);
    } else {
        LerpEnvColor(&g_EnvironmentColors.fields.slots[7].from, &g_EnvironmentColors.fields.slots[7].to,
                     &g_EnvironmentColors.fields.slots[7].cur, frac);
        LerpEnvColor(&g_EnvironmentColors.fields.slots[8].from, &g_EnvironmentColors.fields.slots[8].to,
                     &g_EnvironmentColors.fields.slots[8].cur, frac);
    }

    SetFarColor(g_EnvironmentColors.fields.slots[0].cur.bytes.r,
                g_EnvironmentColors.fields.slots[0].cur.bytes.g,
                g_EnvironmentColors.fields.slots[0].cur.bytes.b);

    if (g_EnvSpareLerp != 0) {
        g_EnvironmentColors.fields.slots[0].cur.bytes.unused = (g_EnvSpareFrom * diff + g_EnvSpareTo * g_EnvLerpFrame) / g_EnvLerpDuration;
    }

    if (g_EnvLerpFrame == g_EnvLerpDuration) {
        if ((g_EnvironmentColors.fogColorWord & 0xFFFF0000) == 0x80800000 &&
            g_EnvironmentColors.fields.slots[0].cur.bytes.b == 0x80) {
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
