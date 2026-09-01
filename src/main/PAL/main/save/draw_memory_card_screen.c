#include "game/prim.h"
#include "game/render_internal.h"
#include "game/memcard.h"
#include "game/menu.h"

void DrawMemoryCardScreen(s32 showBar, s32 variant, s32 cursor, s32 barRow)
{
    OT_TYPE *base = GamePrimaryOrderingTable(51);
    u8 **cursorSlot = &RENDER_PRIM_CURSOR_AS(u8);
    u8 *next;
    s32 i;
    s32 y;

    next = GameQueueSpriteTrans(base, *cursorSlot, 0x24, 0x38, 0x20, 0x18, 0xA0, 0x90, 0x7F40);
    if (variant != 0) {
        next = GameQueueSpriteTrans(base, next, 0x24, 0x58, 0x24, 0x18, 0xCC, 0x90, 0x7F40);
    }
    y = variant != 0 ? 0x78 : 0x58;
    next = GameQueueSpriteTrans(base, next, 0x24, y, 0x1C, 0x18, 0xD0, 0x60, 0x7F40);
    next = GameQueueSpriteTrans(base, next, 0x48, 0xB8, 0x10, 0x10, 0, 0xC8, 0x7F40);
    next = GameQueueSpriteTrans(base, next, 0x68, 0xB8, 0x34, 0x10, 0x10, 0xC8, 0x7F40);
    next = GameQueueSpriteTrans(base, next, 0xB0, 0xB8, 0x14, 0x10, 0x44, 0xC8, 0x7F40);
    *cursorSlot = next;
    DrawMenuCursorArrow(0x14, (cursor * 32) + 0x38);
    DrawOptionHintBar(variant + 5);
    DrawPadTypeHint();

    base = GamePrimaryOrderingTable(54);
    next = AddTilePrim(base, *cursorSlot, 0x5D, 0x3C, 0xE4, 0x40, 0, 0, 0);
    next = AddTilePrim(base, next, 0x5C, 0x3A, 0xE5, 0x44, 0xFF, 0xFF, 0xFF);
    for (i = 0; i < MEMORY_CARD_SAVE_SLOT_COUNT; i++) {
        next = DrawShadowedTile(base, next, 0x3E, 0xD0 + i * 0x30);
    }

    if (showBar != 0) {
        next = AddTilePrim(base, next, 0x3C, ((barRow * 3) << 4) + 0xCC, 0xC8, 0x28, 0x89, 0xFF, 0x76);
    }
    next = AddTilePrim(base, next, 0, 0, 0x140, 0xF0, 0x85, 0x15, 0xE);
    RENDER_PRIM_CURSOR_AS(u8) = next;
}

void DrawMemoryCardMessage(s32 message) {
    s32 index;
    MemoryCardMessageRow *entry;
    s32 x;
    s32 y;
    s16 *table;
    u8 code;
    u8 *next;
    OT_TYPE *base;
    u32 messageRange;
    u32 delta;

    index = message;
    entry = g_McMessageRows[index];
    x = 0x60;
    y = 0x40;
    messageRange = index - 0x10;
    if (messageRange >= 2 && index != 0x12) {
        table = g_McMessageColumnX;
        code = 1;
        do {
            if (code != 1) {
                x = table[code];
                y = 0x60;
            }
            DrawSpriteString(x, y, entry->text, 0x7F81);
            code = entry->column;
            entry++;
        } while (code != 0);
    }

    base = GamePrimaryOrderingTable(51);
    next = RENDER_PRIM_CURSOR_AS(u8);
    if (index == 6 || index == 8 || index == 0xA || index == 0xC) {
        next = GameQueueSprite(base, next, 0xDE, 0x60, 0xC, 0x18, 0x84, 0x48, 0x7F81);
    }
    if (index == 7 || index == 9 || index == 0xB || index == 0xD) {
        next = GameQueueSprite(base, next, 0xAC, 0x60, 0xC, 0x18, 0x84, 0x48, 0x7F81);
    }
    if (index == 5 && (g_SceneTimer & 0x10) != 0) {
        next = GameQueueSprite(base, next, 0x108, 0x60, 0xC, 0x18, 0x90, 0x48, 0x7F81);
    }
    delta = index - 0x10;
    if (delta < 2 || index == 0x12) {
        next = GameQueueSprite(base, next, x, y, 0x6C, 0x18, 0, delta * 0x18, 0x7F81);
        next = QueueDrawModePrim(base, next, 0x3F);
    } else {
        next = QueueDrawModePrim(base, next, 0x3D);
    }
    RENDER_PRIM_CURSOR_AS(u8) = next;
}
