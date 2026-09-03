#include "game/prim.h"
#include "game/render_internal.h"
#include "game/memcard.h"
#include "game/memcard_internal.h"
#include "game/menu.h"
#include "game/state.h"

enum {
    MEMORY_CARD_BUSY_MESSAGE = 5,
    MEMORY_CARD_SLOT_MESSAGE_FIRST = 6,
    MEMORY_CARD_SLOT_MESSAGE_LAST = 13,
    MEMORY_CARD_BANNER_FIRST = 16,
    MEMORY_CARD_BANNER_LAST = 18,
};

static int IsMemoryCardBanner(s32 message) {
    return message >= MEMORY_CARD_BANNER_FIRST &&
           message <= MEMORY_CARD_BANNER_LAST;
}

static void DrawMemoryCardMessageText(s32 message) {
    MemoryCardMessageRow *row = g_McMessageRows[message];
    s32 x = 0x60;
    s32 y = 0x40;
    u8 nextColumn = 1;

    for (;;) {
        if (nextColumn != 1) {
            x = g_McMessageColumnX[nextColumn];
            y = 0x60;
        }
        DrawSpriteString(x, y, row->text, 0x7F81);
        nextColumn = row->column;
        row++;
        if (nextColumn == 0) {
            break;
        }
    }
}

void DrawMemoryCardMessage(s32 message) {
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(51);
    u8 *prim = RENDER_PRIM_CURSOR_AS(u8);

    if (IsMemoryCardBanner(message)) {
        prim = GameQueueSprite(
            ot,
            prim,
            0x60,
            0x40,
            0x6C,
            0x18,
            0,
            (message - MEMORY_CARD_BANNER_FIRST) * 0x18,
            0x7F81);
        prim = QueueDrawModePrim(ot, prim, 0x3F);
    } else {
        DrawMemoryCardMessageText(message);
        if (message >= MEMORY_CARD_SLOT_MESSAGE_FIRST &&
            message <= MEMORY_CARD_SLOT_MESSAGE_LAST) {
            s32 iconX = (message & 1) != 0 ? 0xAC : 0xDE;
            prim = GameQueueSprite(
                ot, prim, iconX, 0x60, 0xC, 0x18, 0x84, 0x48, 0x7F81);
        } else if (message == MEMORY_CARD_BUSY_MESSAGE &&
                   (g_SceneTimer & 0x10) != 0) {
            prim = GameQueueSprite(
                ot, prim, 0x108, 0x60, 0xC, 0x18, 0x90, 0x48, 0x7F81);
        }
        prim = QueueDrawModePrim(ot, prim, 0x3D);
    }
    g_RenderState.packetCursor = prim;
}
