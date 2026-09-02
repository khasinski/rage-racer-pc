#include "game/menu.h"
#include "game/prim.h"
#include "game/render_internal.h"

enum {
    VOLUME_BAR_X = 0x46,
    VOLUME_SEGMENT_FIRST_X = 0x62,
    VOLUME_SEGMENT_SPACING = 8,
};

void DrawVolumeBar(s32 level, s32 y) {
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    u8 *next = RENDER_PRIM_CURSOR_AS(u8);
    s32 segment;

    next = GameQueueSpriteTrans(ot, next, 0x4E, y + 0xA, 0x10, 0xC, 0xB4,
                                0xC4, 0x7F40);
    next = GameQueueSpriteTrans(ot, next, 0xE4, y + 0xA, 0x10, 0xC, 0xC4,
                                0xC4, 0x7F40);
    next = QueueDrawModePrim(ot, next, 0x3A);

    for (segment = 0; segment <= level; segment++) {
        next = GameQueueSprite(ot, next,
                               VOLUME_SEGMENT_FIRST_X +
                                   segment * VOLUME_SEGMENT_SPACING,
                               y + 4, 4, 0x18, 0xFC, 0x40, 0x7E82);
    }

    next = QueueDrawModePrim(ot, next, 0x39);
    next = AddTilePrim(ot, next, VOLUME_BAR_X + 1, y + 2, 0xB2, 0x1C, 0, 0,
                       0);
    RENDER_PRIM_CURSOR_AS(u8) =
        AddTilePrim(ot, next, VOLUME_BAR_X, y, 0xB4, 0x20, 0xFF, 0xFF, 0xFF);
}
