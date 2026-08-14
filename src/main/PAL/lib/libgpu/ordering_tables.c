#include "common.h"
#include "psyq/gpu.h"
#include "game/render.h"
#include "psyq/gpu_internal.h"


void DrawOTag(void *ot) {
    if (g_GraphDebug >= 2) {
        GPU_printf(g_GpuTraceDrawOTag, ot);
    }
    g_GpuFuncs->send(g_GpuFuncs->sendList, ot, 0, 0);
}

/* Named from its own trace string g_GpuTracePutDrawEnv, "PutDrawEnv(%08x)...". */
DrawEnv *PutDrawEnv(DrawEnv *env) {
    u8 *debug = &g_GraphDebug;
    DrawEnv *prim = env;
    DrawEnvPacket *tag;

    if (*debug >= 2) {
        GPU_printf(g_GpuTracePutDrawEnv, prim);
    }

    tag = &prim->packet;
    Gpu_BuildDrawEnvCmds(tag, prim);
    {
        u32 mask = 0xFFFFFF;
        DrawEnvPacket *sendTag = tag;
        s32 size = 0x40;
        u32 word = tag->tag;
        GpuCallbacks *gpu = g_GpuFuncs;

        word |= mask;
        tag->tag = word;
        gpu->send(gpu->sendList, sendTag, size, 0);
    }
    MemCopy(debug + 0xE, (u_char *)prim, 0x5C);
    return prim;
}

void DrawOTagEnv(void *ot, DrawEnv *env) {
    void *src = ot;
    u8 *debug = &g_GraphDebug;
    DrawEnv *prim = env;
    DrawEnvPacket *tag;

    if (*debug >= 2) {
        GPU_printf(g_GpuTraceDrawOTagEnv, src, prim);
    }

    tag = &prim->packet;
    Gpu_BuildDrawEnvCmds(tag, prim);
    {
        u32 word = (tag->tag & 0xFF000000) |
                   ((u32)src & 0xFFFFFF);
        GpuCallbacks *gpu = g_GpuFuncs;

        tag->tag = word;
        gpu->send(gpu->sendList, tag, 0x40, 0);
    }
    MemCopy(debug + 0xE, (u_char *)prim, 0x5C);
}
