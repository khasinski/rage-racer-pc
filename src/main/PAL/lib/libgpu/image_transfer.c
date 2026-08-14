#include "common.h"
#include "psyq/gpu.h"
#include "psyq/gpu_internal.h"

/*
 * The five words at 0x80094290 are one GPU "move image" primitive: tag, the
 * 0xE1-class command word, then source xy, destination xy and the packed
 * width/height.  MoveImage fills the last three and sends the whole packet.
 */


void LoadImage(Rect *rect, void *pixels) {
    CheckPrim(g_GpuNameLoadImage, rect);
    g_GpuFuncs->send(g_GpuFuncs->loadImage, rect, 8, (u_long)pixels);
}

void StoreImage(Rect *rect, void *data) {
    CheckPrim(g_GpuNameStoreImage, rect);
    g_GpuFuncs->send(g_GpuFuncs->storeImage, rect, 8, (u_long)data);
}

long MoveImage(GpuRectPacked *rect, u_long x, u_long y) {
    CheckPrim(g_GpuNameMoveImage, rect);
    if (rect->w == 0 || rect->h == 0) {
        return -1;
    }

    g_MoveImagePacket.src = rect->xy;
    g_MoveImagePacket.dst = (y << 0x10) | (x & 0xFFFF);
    g_MoveImagePacket.wh = *(u_long *)&rect->w;
    return g_GpuFuncs->send(g_GpuFuncs->sendList, &g_MoveImagePacket,
                            sizeof(g_MoveImagePacket), 0);
}


void *ClearOTag(u_long *ot, long count) {
    long remaining = count;

    if (g_GraphDebug >= 2) {
        void (*debug)(char *, ...) = GPU_printf;

        debug(g_FmtGpuClearOTag, ot, remaining);
    }

    remaining--;
    if (remaining != 0) {
        u_long mask = 0xFFFFFF;
        u_long hiMask = 0xFF000000;

        do {
            u_long *next;
            u_long tag;
            u_long low;

            remaining--;
            next = ot + 1;
            ((u_char *)ot)[3] = 0;
            tag = *ot;
            low = (u_long)next & mask;
            tag &= hiMask;
            tag |= low;
            *ot = tag;
            ot = next;
        } while (remaining != 0);
    }

    *ot = (u_long)&g_OtagTerminator & 0xFFFFFF;
    return ot;
}

void *ClearOTagR(u_long *ot, long count) {
    if (g_GraphDebug >= 2) {
        GPU_printf(g_FmtGpuClearOTagR, ot, count);
    }

    g_GpuFuncs->clearOTag(ot, count);

    {
        u_long mask = 0xFFFFFF;
        register u_long *ret asm("$2") = ot;
        u_long next;

        asm("" : "=r"(ret), "=r"(mask) : "0"(ret), "1"(mask));
        next = (u_long)&g_OtagTerminator;
        asm("" : "=r"(next) : "0"(next));
        next &= mask;
        *ret = next;
        return ret;
    }
}


void DrawPrim(u_char *ot) {
    u_long mode = ot[3];

    g_GpuFuncs->drawSync(0);
    g_GpuFuncs->writeGp0Words(ot + 4, mode);
}
