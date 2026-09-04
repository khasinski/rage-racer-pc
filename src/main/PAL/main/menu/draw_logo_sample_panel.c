#include "game/menu.h"
#include "game/menu_internal.h"

enum {
    LOGO_SAMPLE_PANEL_LAST_FRAME = 5,
    LOGO_SAMPLE_PANEL_Y = 494,
    LOGO_SAMPLE_PANEL_FRAME_DISTANCE = 30,
    LOGO_SAMPLE_SWATCH_COUNT = 15,
};

void DrawLogoSamplePanel(s32 step, s32 sample) {
    GameOrderingTableEntry *ot = RENDER_OT_BASE;
    s32 frame;
    s32 y;
    s32 i;

    if (step == 0) {
        g_LogoSamplePanelSlide = 0;
        return;
    }
    if (sample < 0) {
        sample = 0;
    }
    g_LogoSamplePanelSlide = AddClampedMenuValue(
        g_LogoSamplePanelSlide, 0, 0, LOGO_SAMPLE_PANEL_LAST_FRAME);
    if (step < 0) {
        g_LogoSamplePanelSlide = AddClampedMenuValue(
            g_LogoSamplePanelSlide, step, 0, LOGO_SAMPLE_PANEL_LAST_FRAME);
    }
    frame = g_LogoSamplePanelSlide;
    if (step > 0) {
        g_LogoSamplePanelSlide = AddClampedMenuValue(
            g_LogoSamplePanelSlide, step, 0,
            LOGO_SAMPLE_PANEL_LAST_FRAME);
    }
    if (ot == NULL) return;

    y = LOGO_SAMPLE_PANEL_Y - frame * LOGO_SAMPLE_PANEL_FRAME_DISTANCE;

    DrawSprite(ot, 0xDA, (s16)y, 8, 0x10, (u8)((sample / 10) << 3), 0x18,
               0, 0, 0, 0x244, 1, 1, 0x3B);
    DrawSprite(ot, 0xE2, (s16)y, 8, 0x10, (u8)((sample % 10) << 3), 0x18,
               0, 0, 0, 0x244, 1, 1, 0x3B);
    DrawSprite(ot, 0xA2, (s16)y, 4, 0x10, 0x78, 0xCC, 0, 0, 0, 0x244, 1,
               1, 0x3A);
    DrawSprite(ot, 0xA8, (s16)y, 0x34, 0x10, 0x7C, 0xCC, 0, 0, 0, 0x244,
               1, 1, 0x3A);
    DrawSprite(ot, 0xEA, (s16)y, 4, 0x10, 0xB0, 0xCC, 0, 0, 0, 0x244, 1,
               1, 0x3A);

    for (i = 0; i < LOGO_SAMPLE_SWATCH_COUNT; i++) {
        u16 colour = g_TeamLogoSwatches[i];

        DrawSolidRect(ot, 0x8B + i * 8, y + 34, 8, 0x10,
                      (colour & 0x1F) << 3, ((colour >> 5) & 0x1F) << 3,
                      ((colour >> 10) & 0x1F) << 3, 0xFF);
    }
    DrawRectOutline(ot, 0x8A, y + 32, 0x7A, 0x14, 0xB4, 0xB4, 0xB4,
                    0xFF);
}
