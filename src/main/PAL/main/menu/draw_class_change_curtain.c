#include "game/menu.h"

#include <stdint.h>

#define CLASS_CHANGE_CURTAIN_MAX_SLIDE 25
#define CLASS_CHANGE_CURTAIN_MAX_DRAW_PHASE 15
#define CLASS_CHANGE_CURTAIN_HEIGHT 240
#define CLASS_CHANGE_CURTAIN_WIDTH 320

static s32 ClampCurtainSlide(int64_t slide) {
    if (slide < 0) return 0;
    if (slide > CLASS_CHANGE_CURTAIN_MAX_SLIDE) {
        return CLASS_CHANGE_CURTAIN_MAX_SLIDE;
    }
    return (s32)slide;
}

static void DrawClassChangeCurtainPanels(s32 slide) {
    const s32 phase = slide < CLASS_CHANGE_CURTAIN_MAX_DRAW_PHASE
                          ? slide
                          : CLASS_CHANGE_CURTAIN_MAX_DRAW_PHASE;
    const s32 upperPanelY = -CLASS_CHANGE_CURTAIN_HEIGHT + phase * 16;
    const s32 lowerPanelY = CLASS_CHANGE_CURTAIN_HEIGHT - upperPanelY;
    void *ot = RENDER_OT_BASE;

    DrawSolidRect(ot, 0, upperPanelY, CLASS_CHANGE_CURTAIN_WIDTH,
                  CLASS_CHANGE_CURTAIN_HEIGHT, 0x95, 0x25, 0x1E, 0xFF);
    DrawSolidRect(ot, 0, lowerPanelY, CLASS_CHANGE_CURTAIN_WIDTH,
                  CLASS_CHANGE_CURTAIN_HEIGHT, 0x95, 0x25, 0x1E, 0xFF);
}

s32 DrawClassChangeCurtain(s32 step) {
    if (step == 0) {
        g_ClassChangeCurtainSlide = 0;
        return 0;
    }

    if (step < 0) {
        g_ClassChangeCurtainSlide =
            ClampCurtainSlide((int64_t)g_ClassChangeCurtainSlide + step);
    }

    if (g_MenuAltLayout == 0) {
        DrawClassChangeCurtainPanels(g_ClassChangeCurtainSlide);
    }

    if (step > 0) {
        g_ClassChangeCurtainSlide =
            ClampCurtainSlide((int64_t)g_ClassChangeCurtainSlide + step);
    }

    return g_ClassChangeCurtainSlide;
}
