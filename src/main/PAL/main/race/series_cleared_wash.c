#include "game/prim.h"
#include "game/replay_internal.h"
#include "game/render.h"
#include "game/render_internal.h"

#include <stdint.h>

static s32 ClampColorChannel(int64_t value) {
    if (value < 0) {
        return 0;
    }
    return value > 0xFF ? 0xFF : value;
}

void DrawSeriesClearedWash(s32 washProgress, s32 fadeLevel) {
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    s32 red = ClampColorChannel(fadeLevel);
    s32 green = ClampColorChannel(
        (int64_t)fadeLevel + washProgress / 8);
    s32 blue = ClampColorChannel(
        (int64_t)fadeLevel + washProgress / 4);
    u8 *next = GameQueueTileTrans(
        ot, RENDER_PRIM_CURSOR_AS(u8), 0, 0, 0x140, 0xF0,
        red, green, blue);

    g_RenderState.packetCursor = QueueDrawModePrim(ot, next, 0x49);
}
