#include "game/prim.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"

static s32 ClampColorChannel(s32 value) {
    if (value < 0) {
        return 0;
    }
    return value > 0xFF ? 0xFF : value;
}

void DrawSeriesClearedWash(s32 washProgress, s32 fadeLevel) {
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    s32 red = ClampColorChannel(fadeLevel);
    s32 green = ClampColorChannel(fadeLevel + washProgress / 8);
    s32 blue = ClampColorChannel(fadeLevel + washProgress / 4);
    u8 *next = GameQueueTileTrans(
        ot, RENDER_PRIM_CURSOR_AS(u8), 0, 0, 0x140, 0xF0,
        red, green, blue);

    RENDER_PRIM_CURSOR_AS(u8) = QueueDrawModePrim(ot, next, 0x49);
}
