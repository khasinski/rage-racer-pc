#include "game/car.h"
#include "game/menu.h"
#include "game/menu_internal.h"

enum {
    OWNED_CAR_COUNTER_DRAW_START = 11,
    OWNED_CAR_COUNTER_LAST_FRAME = 10,
    OWNED_CAR_COUNTER_COMPLETE = 25,
};

void DrawOwnedCarCounter(s32 direction, s32 ownedCount) {
    s32 frame;

    if (direction == 0) {
        g_OwnedCarCounterSlide = 0;
        return;
    }
    g_OwnedCarCounterSlide = AddClampedMenuValue(
        g_OwnedCarCounterSlide, 0, 0, OWNED_CAR_COUNTER_COMPLETE);
    if (direction < 0) {
        g_OwnedCarCounterSlide = AddClampedMenuValue(
            g_OwnedCarCounterSlide, direction, 0, OWNED_CAR_COUNTER_COMPLETE);
    }

    frame = g_OwnedCarCounterSlide - OWNED_CAR_COUNTER_DRAW_START;
    if (frame >= 0 && g_MenuAltLayout == 0 && RENDER_OT_BASE != NULL) {
        s32 y;
        u32 displayedCount;

        if (frame > OWNED_CAR_COUNTER_LAST_FRAME) {
            frame = OWNED_CAR_COUNTER_LAST_FRAME;
        }
        y = 0x21B - frame * 35;
        displayedCount = (u32)AddClampedMenuValue(
            ownedCount, 0, 0, GAME_CAR_COUNT);

        const s32 numberFlags = DRAW_NUMBER_LARGE_DIGITS |
                                DRAW_NUMBER_TEN_DIGIT_FIELD |
                                DRAW_NUMBER_ALT_DIGIT_ATLAS;
        GameDrawNumber(0x2C, y, numberFlags, displayedCount, 0x7F, 0x7F,
                       0x7F, 0x259, 0x20);
        GameDrawNumber(0x44, y, numberFlags, GAME_CAR_COUNT, 0x7F, 0x7F,
                       0x7F, 0x259, 0x20);
        DrawSprite(RENDER_OT_BASE, 0x17, (s16)y, 0x34, 0x10, 0x8C, 0x8C,
                   0, 0, 0, 0x244, 1, 1, 0x3B);
        DrawSprite(RENDER_OT_BASE, 0x7C, (s16)y, 8, 0x10, 0x8C, 0xDC, 0,
                   0, 0, 0x259, 1, 1, 0x3B);
        GameDrawMenuButton(0, y - 10, 0x99, 0x23, 0, 0, 0);
    }

    if (direction > 0) {
        g_OwnedCarCounterSlide = AddClampedMenuValue(
            g_OwnedCarCounterSlide, direction, 0,
            OWNED_CAR_COUNTER_COMPLETE);
    }
}
