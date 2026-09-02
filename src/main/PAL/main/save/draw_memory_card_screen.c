#include "game/prim.h"
#include "game/render_internal.h"
#include "game/memcard.h"
#include "game/menu.h"

void DrawMemoryCardScreen(s32 showSlotBar, s32 fromLoadMenu,
                          s32 selectedRow, s32 selectedSlot) {
    GameOrderingTableEntry *base = GamePrimaryOrderingTable(51);
    u8 *next = RENDER_PRIM_CURSOR_AS(u8);
    s32 i;
    s32 y;

    next = GameQueueSpriteTrans(
        base, next, 0x24, 0x38, 0x20, 0x18, 0xA0, 0x90, 0x7F40);
    if (fromLoadMenu != 0) {
        next = GameQueueSpriteTrans(
            base, next, 0x24, 0x58, 0x24, 0x18, 0xCC, 0x90, 0x7F40);
    }
    y = fromLoadMenu != 0 ? 0x78 : 0x58;
    next = GameQueueSpriteTrans(
        base, next, 0x24, y, 0x1C, 0x18, 0xD0, 0x60, 0x7F40);
    next = GameQueueSpriteTrans(
        base, next, 0x48, 0xB8, 0x10, 0x10, 0, 0xC8, 0x7F40);
    next = GameQueueSpriteTrans(
        base, next, 0x68, 0xB8, 0x34, 0x10, 0x10, 0xC8, 0x7F40);
    next = GameQueueSpriteTrans(
        base, next, 0xB0, 0xB8, 0x14, 0x10, 0x44, 0xC8, 0x7F40);
    RENDER_PRIM_CURSOR_AS(u8) = next;
    DrawMenuCursorArrow(0x14, selectedRow * 32 + 0x38);
    DrawOptionHintBar(fromLoadMenu + 5);
    DrawPadTypeHint();

    base = GamePrimaryOrderingTable(54);
    next = AddTilePrim(base, next, 0x5D, 0x3C, 0xE4, 0x40, 0, 0, 0);
    next = AddTilePrim(base, next, 0x5C, 0x3A, 0xE5, 0x44, 0xFF, 0xFF, 0xFF);
    for (i = 0; i < MEMORY_CARD_SAVE_SLOT_COUNT; i++) {
        next = DrawShadowedTile(base, next, 0x3E, 0xD0 + i * 0x30);
    }

    if (showSlotBar != 0) {
        next = AddTilePrim(base, next, 0x3C, selectedSlot * 0x30 + 0xCC,
                           0xC8, 0x28, 0x89, 0xFF, 0x76);
    }
    next = AddTilePrim(base, next, 0, 0, 0x140, 0xF0, 0x85, 0x15, 0xE);
    RENDER_PRIM_CURSOR_AS(u8) = next;
}

void DrawMemoryCardMessage(s32 message) {
    MemoryCardMessageRow *row;
    s32 x;
    s32 y;
    u8 column;
    u8 *next;
    GameOrderingTableEntry *base;
    s32 specialMessage;

    row = g_McMessageRows[message];
    x = 0x60;
    y = 0x40;
    specialMessage = message - 0x10;
    if (message < 0x10 || message > 0x12) {
        column = 1;
        do {
            if (column != 1) {
                x = g_McMessageColumnX[column];
                y = 0x60;
            }
            DrawSpriteString(x, y, row->text, 0x7F81);
            column = row->column;
            row++;
        } while (column != 0);
    }

    base = GamePrimaryOrderingTable(51);
    next = RENDER_PRIM_CURSOR_AS(u8);
    if (message >= 6 && message <= 0xD && (message & 1) == 0) {
        next = GameQueueSprite(base, next, 0xDE, 0x60, 0xC, 0x18, 0x84, 0x48, 0x7F81);
    }
    if (message >= 7 && message <= 0xD && (message & 1) != 0) {
        next = GameQueueSprite(base, next, 0xAC, 0x60, 0xC, 0x18, 0x84, 0x48, 0x7F81);
    }
    if (message == 5 && (g_SceneTimer & 0x10) != 0) {
        next = GameQueueSprite(base, next, 0x108, 0x60, 0xC, 0x18, 0x90, 0x48, 0x7F81);
    }
    if (message >= 0x10 && message <= 0x12) {
        next = GameQueueSprite(base, next, x, y, 0x6C, 0x18, 0,
                               specialMessage * 0x18, 0x7F81);
        next = QueueDrawModePrim(base, next, 0x3F);
    } else {
        next = QueueDrawModePrim(base, next, 0x3D);
    }
    RENDER_PRIM_CURSOR_AS(u8) = next;
}
