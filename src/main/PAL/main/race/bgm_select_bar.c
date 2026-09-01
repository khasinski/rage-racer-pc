#include "game/menu.h"
#include "game/prim.h"
#include "game/race.h"
#include "game/render_internal.h"

enum {
    BGM_SELECT_BUTTON_COUNT = 3,
    BGM_SELECT_BUTTON_WIDTH = 0x14,
    BGM_SELECT_BUTTON_HEIGHT = 0x10,
    BGM_SELECT_CLUT_ACTIVE = 0x3FEC,
    BGM_SELECT_CLUT_INACTIVE = 0x3FEF,
};

void DrawBgmSelectBar(void) {
    void *ot = GamePrimaryOrderingTable(1);
    u8 *next = RENDER_PRIM_CURSOR_AS(u8);
    s32 labelV;
    s32 button;

    for (button = 0; button < BGM_SELECT_BUTTON_COUNT; button++) {
        s32 clut = button == g_BgmSelectCursor
                       ? BGM_SELECT_CLUT_ACTIVE
                       : BGM_SELECT_CLUT_INACTIVE;

        next = GameQueueSprite(
            ot, next, 0x20 + button * 0x16, 0xC1,
            BGM_SELECT_BUTTON_WIDTH, BGM_SELECT_BUTTON_HEIGHT,
            button * BGM_SELECT_BUTTON_WIDTH, 0, clut);
    }

    if (g_BgmRandomLabelTimer != 0) {
        g_BgmRandomLabelTimer--;
        labelV = 0x10;
    } else {
        labelV = g_BgmSelectTrack * 12 + 0x1C;
    }

    next = GameQueueSprite(ot, next, 0x64, 0xC2, 0xBA, 0xC, 0, labelV,
                           0x3FED);
    next = GameQueueSprite(ot, next, 0x62, 0xC0, 0xBE, 0x10, 0x3C, 0,
                           0x3FEE);
    next = GameQueueTileTrans(ot, next, 0x14, 0xB8, 0x118, 0x20, 0, 0, 0);
    RENDER_PRIM_CURSOR_AS(u8) = QueueDrawModePrim(ot, next, 0xB);
}
