#include "game/menu.h"

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

    if (step < 0) {
        g_BrowseArrowsFade += step;
        if (g_BrowseArrowsFade < 0) {
            g_BrowseArrowsFade = 0;
        }
    }

    ot = RENDER_OT_BASE_AS(void);
    halfWidth = wide != 0 || g_MenuAltLayout != 0 ? 0x58 : 1;
    y = wide != 0 ? 0x144 : 0x119;
    slidePhase = g_BrowseArrowsFade - BROWSE_ARROWS_VISIBLE_AT;
    if (slidePhase >= 0) {
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
        intensity = (u8)(rsin(g_BrowseArrowsPulsePhase % 0x1000) / 64 - 65);
        g_BrowseArrowsPulsePhase += BROWSE_ARROWS_PULSE_STEP;

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
        g_BrowseArrowsFade += step;
        if (g_BrowseArrowsFade > BROWSE_ARROWS_FADE_MAX) {
            g_BrowseArrowsFade = BROWSE_ARROWS_FADE_MAX;
        }
    }
}
