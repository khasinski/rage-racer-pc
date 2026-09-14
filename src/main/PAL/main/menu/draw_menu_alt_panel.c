#include "game/menu_internal.h"

enum {
    MENU_UPPER_ALT_PANEL_MAX_PROGRESS = 14,
    MENU_LOWER_ALT_PANEL_MAX_PROGRESS = 16,
};

static void DrawUpperAltPanel(GameOrderingTableEntry *ot, s32 progress) {
    const s32 verticalOffset = (progress - 1) * 2;
    const s32 left = g_MenuAltLayout != 0 ? 0x69 : 0xA8;
    const s32 right = left + 0x1C;
    const s32 top = 0x9E - verticalOffset;
    const s32 bottom = 0x9F + verticalOffset;

    GameDrawTexturedQuad(ot, left, top, right, top, left, bottom, right,
                         bottom, 0xB0, 0x38, 0xCC, 0x38, 0xB0, 0x6C, 0xCC,
                         0x6C, 0x7F, 0x7F, 0x7F, 0x232, 0, 0, 0x1C);
}

static void DrawLowerAltPanel(GameOrderingTableEntry *ot, s32 progress) {
    const s32 verticalOffset = progress - 1;
    const s32 left = g_MenuAltLayout != 0 ? 0x92 : 0xC0;
    const s32 right = left + 0x4E;
    const s32 top = 0x128 - verticalOffset;
    const s32 bottom = 0x128 + progress;

    GameDrawTexturedQuad(ot, left, top, right, top, left, bottom, right,
                         bottom, 0x61, 0x38, 0xAF, 0x38, 0x61, 0x58, 0xAF,
                         0x58, 0x7F, 0x7F, 0x7F, 0x259, 0, 0, 0x1C);
}

void DrawMenuAltPanel(MenuWidgets *widgets) {
    GameOrderingTableEntry *ot = RENDER_OT_BASE;
    s32 upperStep = widgets->upperAltPanelStep;
    s32 lowerStep = widgets->lowerAltPanelStep;

    if (upperStep == 0 && lowerStep == 0) {
        widgets->upperAltPanel = 0;
        widgets->lowerAltPanel = 0;
        return;
    }

    if (upperStep < 0) {
        widgets->upperAltPanel = AddClampedMenuValue(
            widgets->upperAltPanel, upperStep,
            0, MENU_UPPER_ALT_PANEL_MAX_PROGRESS);
    }
    if (lowerStep < 0) {
        widgets->lowerAltPanel = AddClampedMenuValue(
            widgets->lowerAltPanel, lowerStep,
            0, MENU_LOWER_ALT_PANEL_MAX_PROGRESS);
    }

    if (widgets->upperAltPanel != 0 && ot != NULL) {
        DrawUpperAltPanel(ot, widgets->upperAltPanel);
    }
    if (widgets->lowerAltPanel != 0 && ot != NULL) {
        DrawLowerAltPanel(ot, widgets->lowerAltPanel);
    }

    if (upperStep > 0) {
        widgets->upperAltPanel = AddClampedMenuValue(
            widgets->upperAltPanel, upperStep,
            0, MENU_UPPER_ALT_PANEL_MAX_PROGRESS);
    }
    if (lowerStep > 0) {
        widgets->lowerAltPanel = AddClampedMenuValue(
            widgets->lowerAltPanel, lowerStep,
            0, MENU_LOWER_ALT_PANEL_MAX_PROGRESS);
    }
}
