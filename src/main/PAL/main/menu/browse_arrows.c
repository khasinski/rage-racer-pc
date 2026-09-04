#include "game/menu.h"
#include "game/menu_internal.h"

enum {
    BROWSE_ARROWS_VISIBLE_AT = 11,
    BROWSE_ARROWS_SLIDE_FRAMES = 11,
    BROWSE_ARROWS_FADE_MAX = 25,
    BROWSE_ARROWS_PULSE_STEP = 0x60,
    BROWSE_ARROWS_COMPACT_HALF_WIDTH = 1,
    BROWSE_ARROWS_WIDE_HALF_WIDTH = 0x58,
    BROWSE_ARROWS_COMPACT_Y = 0x119,
    BROWSE_ARROWS_COURSE_Y = 0x144,
    BROWSE_ARROWS_RIGHT_ORIGIN = 0x1BF,
    BROWSE_ARROWS_SPRITE_WIDTH = 0x10,
    BROWSE_ARROWS_SPRITE_HEIGHT = 0x20,
};

/* The two side browse arrows, each lit only when that direction has somewhere
 * to go. Positive steps fade in after drawing; negative steps fade out before
 * drawing, matching the other menu widgets. */
void DrawBrowseArrows(s32 step, s32 courseLayout, s32 drawLeft,
                      s32 drawRight) {
    GameOrderingTableEntry *ot;
    s32 halfWidth;
    s32 y;
    s32 slidePhase;

    if (step == 0) {
        g_BrowseArrowsFade = 0;
        return;
    }

    g_BrowseArrowsFade = AddClampedMenuValue(
        g_BrowseArrowsFade, 0, 0, BROWSE_ARROWS_FADE_MAX);

    if (step < 0) {
        g_BrowseArrowsFade = AddClampedMenuValue(
            g_BrowseArrowsFade, step, 0, BROWSE_ARROWS_FADE_MAX);
    }

    halfWidth = courseLayout != 0 || g_MenuAltLayout != 0
                    ? BROWSE_ARROWS_WIDE_HALF_WIDTH
                    : BROWSE_ARROWS_COMPACT_HALF_WIDTH;
    y = courseLayout != 0 ? BROWSE_ARROWS_COURSE_Y
                          : BROWSE_ARROWS_COMPACT_Y;
    slidePhase = g_BrowseArrowsFade - BROWSE_ARROWS_VISIBLE_AT;
    if (slidePhase >= 0 && RENDER_OT_BASE != NULL) {
        s32 leftX;
        s16 leftEdge;
        s16 rightEdge;
        u8 intensity;

        if (slidePhase >= BROWSE_ARROWS_SLIDE_FRAMES) {
            slidePhase = BROWSE_ARROWS_SLIDE_FRAMES - 1;
        }
        leftX = -25 + (slidePhase * 0x250) / 32;
        leftEdge = leftX - halfWidth;
        rightEdge = BROWSE_ARROWS_RIGHT_ORIGIN - leftX;
        intensity =
            (u8)(rsin((s32)((u32)g_BrowseArrowsPulsePhase & 0xFFFu)) / 64 -
                 65);
        g_BrowseArrowsPulsePhase =
            (s32)((u32)g_BrowseArrowsPulsePhase + BROWSE_ARROWS_PULSE_STEP);
        ot = RENDER_OT_BASE;

        DrawSprite(ot, leftEdge, y, BROWSE_ARROWS_SPRITE_WIDTH,
                   BROWSE_ARROWS_SPRITE_HEIGHT, 0x48, 0xB8, 0, 0, 0, 0x25A,
                   1, 0, 0x19);
        DrawSprite(ot, rightEdge, y, BROWSE_ARROWS_SPRITE_WIDTH,
                   BROWSE_ARROWS_SPRITE_HEIGHT, 0x58, 0xB8, 0, 0, 0, 0x25A,
                   1, 0, 0x19);
        if (drawLeft != 0) {
            DrawSolidRect(ot, leftEdge, y, BROWSE_ARROWS_SPRITE_WIDTH,
                          BROWSE_ARROWS_SPRITE_HEIGHT, 0, intensity, 0, 0xFF);
        }
        if (drawRight != 0) {
            DrawSolidRect(ot, rightEdge, y, BROWSE_ARROWS_SPRITE_WIDTH,
                          BROWSE_ARROWS_SPRITE_HEIGHT, 0, intensity, 0, 0xFF);
        }
    }

    if (step > 0) {
        g_BrowseArrowsFade = AddClampedMenuValue(
            g_BrowseArrowsFade, step, 0, BROWSE_ARROWS_FADE_MAX);
    }
}
