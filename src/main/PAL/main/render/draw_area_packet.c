#include "game/prim.h"
#include "game/render.h"
#include "game/render_internal.h"

/* Queue a drawing-area change. The second framebuffer occupies the lower
 * 240 lines of VRAM, so its clipping rectangle needs the same y offset. */
u8 *QueueDrawAreaPrim(GameOrderingTableEntry *ot, DrawPacket *packet, s16 x, s16 y, s32 width,
                      s32 height) {
    Rect rect = {x, y + g_FrameParity * 240, width, height};

    SetDrawArea(packet, &rect);
    AddPrim(ot, packet);
    return (u8 *)(packet + 1);
}
