#include "common.h"
#include "game/menu.h"
#include "game/render.h"
#include "game/render_workspace.h"
void DrawLogoSamplePanel(s32 step, s32 sample) {
    void *ot = RENDER_OT_BASE_AS(void);
    s32 idx;
    u32 t;
    s16 y;
    s32 i;
    s32 xoff;
    s32 xc = 0xA2;

    if (step == 0) {
        g_LogoSamplePanelSlide = 0;
        return;
    }
    if (step < 0) {
        g_LogoSamplePanelSlide += step;
        if (g_LogoSamplePanelSlide < 0) {
            g_LogoSamplePanelSlide = 0;
        }
    }
    idx = g_LogoSamplePanelSlide;
    if (idx >= 0) {
        if (idx >= 6) {
            idx = 5;
        }
        t = (u32) - (idx * 960) >> 5;
        y = t + 494;
        DrawSprite(ot, 0xDA, y, 8, 0x10, (u8)((sample / 10) << 3), 0x18, 0, 0, 0, 0x244, 1, 1, 0x3B);
        DrawSprite(ot, 0xE2, y, 8, 0x10, (u8)((sample % 10) << 3), 0x18, 0, 0, 0, 0x244, 1, 1, 0x3B);
        DrawSprite(ot, 0xA2, y, 4, 0x10, 0x78, 0xCC, 0, 0, 0, 0x244, 1, 1, 0x3A);
        DrawSprite(ot, 0xA8, y, 0x34, 0x10, 0x7C, 0xCC, 0, 0, 0, 0x244, 1, 1, 0x3A);
        DrawSprite(ot, 0xEA, y, 4, 0x10, 0xB0, 0xCC, 0, 0, 0, 0x244, 1, 1, 0x3A);

        for (i = 0, xoff = -23; i < 15; i++) {
            DrawSolidRect(ot, (s16)(xc + xoff), (s16)(t + 528), 8, (u16)0x10,
                          (u8)((g_TeamLogoSwatches[i] & 0xFF) << 3),
                          (u8)((g_TeamLogoSwatches[i] >> 2) & 0xf8),
                          (u8)((g_TeamLogoSwatches[i] >> 7) & 0xf8), (u8)0xFF);
            xoff += 8;
        }

        DrawRectOutline(ot, (s16)(xc - 24), (s16)((u16)y + 32), 0x7A, 0x14, 0xB4, 0xB4, 0xB4, 0xFF);
    }
    if (step > 0) {
        g_LogoSamplePanelSlide += step;
        if (g_LogoSamplePanelSlide >= 6) {
            g_LogoSamplePanelSlide = 5;
        }
    }
}

void DrawTeamNameEntry(s32 step, s32 cursorIndex) {
    s16 gridY;
    s16 height;
    s16 nameY;
    s16 rowY;
    OT_TYPE *ot;
    s32 lowered;
    s32 raised;
    s32 rounded;
    s32 i;
    s32 rowI;
    u16 rowX;
    u16 xFirst;
    u16 xSecond;
    s32 animationStep;
    s32 sine;
    s32 colour;
    u32 rise;
    u8 ch;

    ot = RENDER_OT_BASE;
    if (step == 0) {
        g_TeamNameEntrySlide = 0;
        return;
    }
    if (step < 0) {
        lowered = step + g_TeamNameEntrySlide;
        g_TeamNameEntrySlide = lowered;
        if (lowered < 0) {
            g_TeamNameEntrySlide = 0;
        }
    }
    if ((g_TeamNameEntrySlide >= 0x19) && ((u8)g_TeamNameLength < 6U)) {
        DrawSprite(ot, (g_TeamNameLength * 0xC) + 0x53, 0x7D, 0xC, 0x18, 0xF4, 0x28, 0,
                       0, 0, 0x244, 1, 1, 0x39);
    }
    animationStep = g_TeamNameEntrySlide - 0xE;
    if (animationStep >= 0) {
        if (animationStep >= 0xC) {
            animationStep = 0xB;
        }
        rowY = ((u32)-(animationStep * 64) >> 5) + 0xFB;
        rounded = g_TeamNameCursorPhase;
        if (g_TeamNameCursorPhase < 0) {
            rounded = g_TeamNameCursorPhase + 0xFFF;
        }
        sine = rsin(g_TeamNameCursorPhase - ((rounded >> 0xC) << 0xC));
        if (sine < 0) {
            sine += 0x3F;
        }
        colour = sine;
        colour >>= 6;
        colour -= 0x41;
        DrawSolidRect(ot + 1, (s16)(((cursorIndex % 11) * 0xC) + 0x54),
                     (s16)(rowY + ((cursorIndex / 11) * 0x18)), 0xB,
                     (s16)(animationStep * 2), 0, colour & 0xFF, 0, 0xFF);
        g_TeamNameCursorPhase += 0x60;
    }
    animationStep = g_TeamNameEntrySlide - 0x11;
    if (animationStep >= 0) {
        if (animationStep >= 9) {
            animationStep = 8;
        }
        gridY = ((u32)-(animationStep * 64) >> 5) + 0xF9;
        if (cursorIndex < 0xA) {
            DrawSprite(ot, (s16)(((cursorIndex % 11) * 0xC) + 0x56),
                           (s16)(gridY + ((cursorIndex / 11) * 0x18)), 8,
                           (s16)(animationStep * 2), (s16)((cursorIndex % 32) * 8),
                           (s16)((cursorIndex / 32) * 16 + 0x18), 0, 0, 0, 0x244, 1, 1, 0x5B);
        } else if (cursorIndex >= 0xB) {
            DrawSprite(ot, (s16)(((cursorIndex % 11) * 0xC) + 0x56),
                           (s16)(gridY + ((cursorIndex / 11) * 0x18)), 8,
                           (s16)(animationStep * 2),
                           (s16)(((cursorIndex - 1) % 32) * 8),
                           (s16)(((cursorIndex - 1) / 32) * 16 + 0x18), 0, 0, 0, 0x244, 1, 1,
                           0x5B);
        }
        rowI = 0;
        do {
            rowX = (rowI * 0xC) + 0x56;
            DrawSprite(ot + 1, (rowX << 16) >> 16, gridY, 8,
                           (s16)(animationStep * 2), (s16)(rowI * 8), 0x18, 0, 0, 0,
                           0x244, 1, 1, 0x3B);
            rowI += 1;
        } while (rowI < 0xA);
        i = 0;
        do {
            rowX = (i * 0xC) + 0x56;
            DrawSprite(ot + 1, (rowX << 16) >> 16, (s16)(gridY + 0x18), 8,
                           (s16)(animationStep * 2), (s16)((i * 8) + 0x50), 0x18, 0,
                           0, 0, 0x244, 1, 1, 0x3B);
            i += 1;
        } while (i < 0xB);
        i = 0;
        do {
            rowX = (i * 0xC) + 0x56;
            DrawSprite(ot + 1, (rowX << 16) >> 16, (s16)(gridY + 0x30), 8,
                           (s16)(animationStep * 2), (s16)((i * 8) + 0xA8), 0x18, 0,
                           0, 0, 0x244, 1, 1, 0x3B);
            i += 1;
        } while (i < 0xB);
        i = 0;
        do {
            rowX = (i * 0xC) + 0x56;
            DrawSprite(ot + 1, (rowX << 16) >> 16, (s16)(gridY + 0x48), 8,
                           (s16)(animationStep * 2), (s16)(i * 8), 0x28, 0, 0, 0,
                           0x244, 1, 1, 0x3B);
            i += 1;
        } while (i < 0xB);
    }
    animationStep = g_TeamNameEntrySlide - 0x13;
    if (animationStep >= 0) {
        if (animationStep >= 7) {
            animationStep = 6;
        }
        rise = (u32)-(animationStep * 64) >> 5;
        height = animationStep * 2;
        DrawSprite(ot + 1, 0xDA, (s16)(rise + 0xF4), 0x1E, height, 0xAC,
                       0xE8, 0, 0, 0, 0x244, 1, 1, 0x3A);
        DrawSprite(ot + 1, 0xDA, (s16)(rise + 0x10B), 0x20, height, 0xAC,
                       0xF4, 0, 0, 0, 0x244, 1, 1, 0x3A);
        DrawSprite(ot + 1, 0xDA, (s16)(rise + 0x117), 0x14, height, 0xCC,
                       0xF4, 0, 0, 0, 0x244, 1, 1, 0x3A);
    }
    animationStep = g_TeamNameEntrySlide - 0x11;
    if (animationStep >= 0) {
        if (animationStep >= 9) {
            animationStep = 8;
        }
        i = 0;
        nameY = ((u32)-(animationStep * 0x178) >> 5) + 0xF9;
        if (i < g_TeamNameLength) {
            do {
                ch = g_TeamNameChars[i];
                if (ch < 0xAU) {
                    xFirst = (i * 0xC) + 0x56;
                    DrawSprite(ot, (xFirst << 16) >> 16, nameY, 8,
                                   (s16)(animationStep * 2),
                                   (s16)((g_TeamNameChars[i] & 0x1F) * 8),
                                   (s16)((g_TeamNameChars[i] >> 5) * 16 + 0x18), 0, 0, 0,
                                   0x244, 1, 1, 0x3B);
                } else if (ch >= 0xBU) {
                    xSecond = (i * 0xC) + 0x56;
                    DrawSprite(ot, (xSecond << 16) >> 16, nameY, 8,
                                   (s16)(animationStep * 2),
                                   (s16)(((g_TeamNameChars[i] - 1) % 32) * 8),
                                   (s16)(((g_TeamNameChars[i] - 1) / 32) * 16 + 0x18), 0, 0, 0,
                                   0x244, 1, 1, 0x3B);
                }
                i += 1;
            } while (i < g_TeamNameLength);
        }
    }
    if (step > 0) {
        raised = step + g_TeamNameEntrySlide;
        g_TeamNameEntrySlide = raised;
        if (raised >= 0x1A) {
            g_TeamNameEntrySlide = 0x19;
        }
    }
}
