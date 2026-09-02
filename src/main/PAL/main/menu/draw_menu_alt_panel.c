#include "game/menu.h"
#include "game/render.h"
#include "game/render_state.h"

#include <stdint.h>

#define MENU_ALT_PANEL_A_MAX_PROGRESS 14
#define MENU_ALT_PANEL_B_MAX_PROGRESS 16

static s32 AdvancePanelProgress(s32 progress, s32 step, s32 maximum) {
    const int64_t advanced = (int64_t)progress + step;
    if (advanced < 0) return 0;
    if (advanced > maximum) return maximum;
    return (s32)advanced;
}

static void DrawUpperAltPanel(void *ot, s32 progress) {
    const s32 verticalOffset = (progress - 1) * 2;
    const s32 left = g_MenuAltLayout != 0 ? 0x69 : 0xA8;
    const s32 right = left + 0x1C;
    const s32 top = 0x9E - verticalOffset;
    const s32 bottom = 0x9F + verticalOffset;

    GameDrawTexturedQuad(ot, left, top, right, top, left, bottom, right,
                         bottom, 0xB0, 0x38, 0xCC, 0x38, 0xB0, 0x6C, 0xCC,
                         0x6C, 0x7F, 0x7F, 0x7F, 0x232, 0, 0, 0x1C);
}

static void DrawLowerAltPanel(void *ot, s32 progress) {
    const s32 verticalOffset = progress - 1;
    const s32 left = g_MenuAltLayout != 0 ? 0x92 : 0xC0;
    const s32 right = left + 0x4E;
    const s32 top = 0x128 - verticalOffset;
    const s32 bottom = 0x128 + progress;

    GameDrawTexturedQuad(ot, left, top, right, top, left, bottom, right,
                         bottom, 0x61, 0x38, 0xAF, 0x38, 0x61, 0x58, 0xAF,
                         0x58, 0x7F, 0x7F, 0x7F, 0x259, 0, 0, 0x1C);
}

void DrawMenuAltPanel(s32 upperStep, s32 lowerStep) {
    void *ot = RENDER_OT_BASE;

    if (upperStep == 0 && lowerStep == 0) {
        g_MenuAltPanelProgressA = 0;
        g_MenuAltPanelProgressB = 0;
        return;
    }

    if (upperStep < 0) {
        g_MenuAltPanelProgressA = AdvancePanelProgress(
            g_MenuAltPanelProgressA, upperStep,
            MENU_ALT_PANEL_A_MAX_PROGRESS);
    }
    if (lowerStep < 0) {
        g_MenuAltPanelProgressB = AdvancePanelProgress(
            g_MenuAltPanelProgressB, lowerStep,
            MENU_ALT_PANEL_B_MAX_PROGRESS);
    }

    if (g_MenuAltPanelProgressA != 0) {
        DrawUpperAltPanel(ot, g_MenuAltPanelProgressA);
    }
    if (g_MenuAltPanelProgressB != 0) {
        DrawLowerAltPanel(ot, g_MenuAltPanelProgressB);
    }

    if (upperStep > 0) {
        g_MenuAltPanelProgressA = AdvancePanelProgress(
            g_MenuAltPanelProgressA, upperStep,
            MENU_ALT_PANEL_A_MAX_PROGRESS);
    }
    if (lowerStep > 0) {
        g_MenuAltPanelProgressB = AdvancePanelProgress(
            g_MenuAltPanelProgressB, lowerStep,
            MENU_ALT_PANEL_B_MAX_PROGRESS);
    }
}
