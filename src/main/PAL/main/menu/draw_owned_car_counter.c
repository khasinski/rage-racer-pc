#include "game/menu.h"
#include "game/menu_internal.h"

enum {
    OWNED_CAR_COUNTER_DRAW_START = 11,
    OWNED_CAR_COUNTER_LAST_FRAME = 10,
    OWNED_CAR_COUNTER_COMPLETE = 25,
};

void DrawOwnedCarCounter(s32 direction, s32 ownedCount) {
    void *ot = RENDER_OT_BASE;
    s32 frame;

    if (direction == 0) {
        g_OwnedCarCounterSlide = 0;
        return;
    }
    if (direction < 0) {
        g_OwnedCarCounterSlide += direction;
        if (g_OwnedCarCounterSlide < 0) {
            g_OwnedCarCounterSlide = 0;
        }
    }

    frame = g_OwnedCarCounterSlide - OWNED_CAR_COUNTER_DRAW_START;
    if (frame >= 0 && g_MenuAltLayout == 0) {
        s32 y;

        if (frame > OWNED_CAR_COUNTER_LAST_FRAME) {
            frame = OWNED_CAR_COUNTER_LAST_FRAME;
        }
        y = 0x21B - frame * 35;

        GameDrawNumber(0x2C, y, 7, ownedCount, 0x7F, 0x7F, 0x7F, 0x259,
                       0x20);
        GameDrawNumber(0x44, y, 7, 0xD, 0x7F, 0x7F, 0x7F, 0x259, 0x20);
        DrawSprite(ot, 0x17, (s16)y, 0x34, 0x10, 0x8C, 0x8C, 0, 0, 0,
                   0x244, 1, 1, 0x3B);
        DrawSprite(ot, 0x7C, (s16)y, 8, 0x10, 0x8C, 0xDC, 0, 0, 0, 0x259,
                   1, 1, 0x3B);
        GameDrawMenuButton(0, y - 10, 0x99, 0x23, 0, 0, 0);
    }

    if (direction > 0) {
        g_OwnedCarCounterSlide += direction;
        if (g_OwnedCarCounterSlide >= OWNED_CAR_COUNTER_COMPLETE) {
            g_OwnedCarCounterSlide = OWNED_CAR_COUNTER_COMPLETE;
        }
    }
}
