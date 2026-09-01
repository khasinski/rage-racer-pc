#include "game/diagnostics.h"
#include "game/render_internal.h"
#include "game/render.h"

#include <stdio.h>
#include <stdlib.h>
#include "game/state.h"


/* DR_MODE, 12 bytes: sets the texture page (and the blend mode packed into it)
 * for the primitives that follow, links it into the ordering table and returns
 * the advanced packet cursor. */
u8 *QueueDrawModePrim(void *ot, u8 *prim, s32 tpage) {
    RenderBufferAddress cursor;
    u8 *pkt;

    cursor.bytes = prim;
    SetDrawMode(cursor.drawPacket, 0, 1, (u16)tpage, g_DrawModeEnv);
    pkt = prim;
    prim += sizeof(DrawPacket);
    AddPrim(ot, pkt);
    return prim;
}

u8 *GameQueueShadedTexturedRect(void *ot, u8 *prim, s32 x, s32 y, s32 w,
                                s32 h, s32 u, s32 v, s32 clutIndex, s32 tpage,
                                s32 intensity) {
    POLY_FT4 *packet;
    RenderBufferAddress packetAddress;
    s16 width = w;
    s16 height = h;
    u8 u0 = u;
    u8 v0 = v;

    packetAddress.bytes = prim;
    SetPolyFT4(packetAddress.polyFT4);
    if (width < 0) {
        width += 1;
        u0 -= width;
    }
    if (height < 0) {
        height += 1;
        v0 -= height;
    }

    packetAddress.bytes = prim;
    packet = packetAddress.polyFT4;
    packet->x0 = x;
    packet->y0 = y;
    packet->x1 = x + (width < 0 ? -width : width);
    packet->y1 = y;
    packet->x2 = x;
    packet->y2 = y + (height < 0 ? -height : height);
    packet->x3 = x + (width < 0 ? -width : width);
    packet->y3 = y + (height < 0 ? -height : height);
    packet->u0 = u0;
    packet->v0 = v0;
    packet->u1 = u0 + width;
    packet->v1 = v0;
    packet->u2 = u0;
    packet->v2 = v0 + height;
    packet->u3 = u0 + width;
    packet->v3 = v0 + height;
    packet->r0 = intensity;
    packet->g0 = intensity;
    packet->b0 = intensity;
    packet->clut = clutIndex;
    packet->tpage = tpage;
    prim += sizeof(POLY_FT4);
    AddPrim(ot, packet);
    return prim;
}

u8 *GameQueueTexturedRect(void *ot, u8 *prim, s32 x, s32 y, s32 w, s32 h,
                          s32 u, s32 v, s32 uSpan, s32 vSpan, s32 clutIndex,
                          s32 tpage) {
    POLY_FT4 *packet;
    RenderBufferAddress packetAddress;
    s16 width = w;
    s16 height = h;
    u8 u0 = u;
    u8 v0 = v;

    packetAddress.bytes = prim;
    SetPolyFT4(packetAddress.polyFT4);
    packetAddress.bytes = prim;
    SetShadeTex(packetAddress.polyFT4, 1);

    if (width < 0) {
        u0 -= width + 1;
    }
    if (height < 0) {
        v0 -= height + 1;
    }

    packetAddress.bytes = prim;
    packet = packetAddress.polyFT4;
    packet->x0 = x;
    packet->y0 = y;
    packet->x1 = x + (width < 0 ? -width : width);
    packet->y1 = y;
    packet->x2 = x;
    packet->y2 = y + (height < 0 ? -height : height);
    packet->x3 = x + (width < 0 ? -width : width);
    packet->y3 = y + (height < 0 ? -height : height);
    packet->u0 = u0;
    packet->v0 = v0;
    packet->u1 = u0 + uSpan;
    packet->v1 = v0;
    packet->u2 = u0;
    packet->v2 = v0 + vSpan;
    packet->u3 = u0 + uSpan;
    packet->v3 = v0 + vSpan;
    packet->clut = clutIndex;
    packet->tpage = tpage;
    prim += sizeof(POLY_FT4);
    AddPrim(ot, packet);
    return prim;
}

/* World position in full-precision components; the camera keeps one of these
 * in the render state. */
/* The per-frame render state: the camera position and the view matrix. */
#define SCRATCH_CAMERA_POS (&RENDER_VIEW_STATE->position.vector)
#define SCRATCH_VIEW_MATRIX ((&g_RenderState.matrix))

/*
 * Per-object GTE setup: takes the object's offset from the camera through the
 * render state's view matrix, scales the rotated offset by 4 into the work
 * matrix's translation, and programs the GTE with the caller's rotation and
 * that translation.
 */
void SetGteObjectMatrix(ObjectMatrixWork *w, LVec *pos, Matrix *rot) {
    w->relative[0] = pos->x - SCRATCH_CAMERA_POS->x;
    w->relative[1] = pos->y - SCRATCH_CAMERA_POS->y;
    w->relative[2] = pos->z - SCRATCH_CAMERA_POS->z;
    ApplyMatrix(SCRATCH_VIEW_MATRIX, w->relative, &w->view);
    w->mtx.t[0] = w->view.x * 4;
    w->mtx.t[1] = w->view.y * 4;
    w->mtx.t[2] = w->view.z * 4;
    SetRotMatrix(rot);
    SetTransMatrix(&w->mtx);
    if (DiagnosticsEnabled("render.car_draw_trace") &&
        g_RenderState.mode == 9) {
        const char *timerText = DiagnosticsValue("render.car_draw_trace_timer");
        if (timerText == NULL || g_SceneTimer == (s32)strtol(timerText, NULL, 0)) {
            Trace("object-matrix", "timer=%d position=%d,%d,%d relative=%d,%d,%d "
                   "view=%d,%d,%d rotation=%d,%d,%d,%d,%d,%d,%d,%d,%d "
                   "translation=%d,%d,%d", g_SceneTimer,
                   pos->x, pos->y, pos->z, w->relative[0], w->relative[1],
                   w->relative[2], w->view.x, w->view.y, w->view.z,
                   rot->m[0][0], rot->m[0][1], rot->m[0][2],
                   rot->m[1][0], rot->m[1][1], rot->m[1][2],
                   rot->m[2][0], rot->m[2][1], rot->m[2][2],
                   w->mtx.t[0], w->mtx.t[1], w->mtx.t[2]);
        }
    }
}
