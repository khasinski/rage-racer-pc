#include "game/menu.h"
#include "game/menu_internal.h"

enum {
    BROWSE_ARROWS_VISIBLE_AT = 11,
    BROWSE_ARROWS_SLIDE_FRAMES = 11,
    BROWSE_ARROWS_FADE_MAX = 25,
    BROWSE_ARROWS_PULSE_STEP = 0x60
};

/* The two side browse arrows, each lit only when that direction has somewhere
 * to go. Positive steps fade in after drawing; negative steps fade out before
 * drawing, matching the other menu widgets. */
void DrawBrowseArrows(s32 step, s32 wide, s32 drawLeft, s32 drawRight) {
    void *ot;
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

    halfWidth = wide != 0 || g_MenuAltLayout != 0 ? 0x58 : 1;
    y = wide != 0 ? 0x144 : 0x119;
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
        rightEdge = 0x1BF - leftX;
        intensity =
            (u8)(rsin((s32)((u32)g_BrowseArrowsPulsePhase & 0xFFFu)) / 64 -
                 65);
        g_BrowseArrowsPulsePhase =
            (s32)((u32)g_BrowseArrowsPulsePhase + BROWSE_ARROWS_PULSE_STEP);
        ot = RENDER_OT_BASE;

        DrawSprite(ot, leftEdge, y, 0x10, 0x20, 0x48, 0xB8, 0, 0, 0, 0x25A,
                   1, 0, 0x19);
        DrawSprite(ot, rightEdge, y, 0x10, 0x20, 0x58, 0xB8, 0, 0, 0, 0x25A,
                   1, 0, 0x19);
        if (drawLeft != 0) {
            DrawSolidRect(ot, leftEdge, y, 0x10, 0x20, 0, intensity, 0, 0xFF);
        }
        if (drawRight != 0) {
            DrawSolidRect(ot, rightEdge, y, 0x10, 0x20, 0, intensity, 0,
                          0xFF);
        }
    }

    if (step > 0) {
        g_BrowseArrowsFade = AddClampedMenuValue(
            g_BrowseArrowsFade, step, 0, BROWSE_ARROWS_FADE_MAX);
    }
}
