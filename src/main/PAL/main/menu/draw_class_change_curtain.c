#include "game/menu_internal.h"

enum {
    CLASS_CHANGE_CURTAIN_MAX_SLIDE = 25,
    CLASS_CHANGE_CURTAIN_MAX_DRAW_PHASE = 15,
    CLASS_CHANGE_CURTAIN_HEIGHT = 240,
    CLASS_CHANGE_CURTAIN_WIDTH = 320,
};

static void DrawClassChangeCurtainPanels(s32 slide) {
    const s32 phase = slide < CLASS_CHANGE_CURTAIN_MAX_DRAW_PHASE
                          ? slide
                          : CLASS_CHANGE_CURTAIN_MAX_DRAW_PHASE;
    const s32 upperPanelY = -CLASS_CHANGE_CURTAIN_HEIGHT + phase * 16;
    const s32 lowerPanelY = CLASS_CHANGE_CURTAIN_HEIGHT - upperPanelY;
    GameOrderingTableEntry *ot = RENDER_OT_BASE;

    if (ot == NULL) return;

    DrawSolidRect(ot, 0, upperPanelY, CLASS_CHANGE_CURTAIN_WIDTH,
                  CLASS_CHANGE_CURTAIN_HEIGHT, 0x95, 0x25, 0x1E, 0xFF);
    DrawSolidRect(ot, 0, lowerPanelY, CLASS_CHANGE_CURTAIN_WIDTH,
                  CLASS_CHANGE_CURTAIN_HEIGHT, 0x95, 0x25, 0x1E, 0xFF);
}

s32 DrawClassChangeCurtain(CourseSelectScreen *screen, s32 step) {
    if (step == 0) {
        screen->curtainSlide = 0;
        return 0;
    }

    if (step < 0) {
        screen->curtainSlide = AddClampedMenuValue(
            screen->curtainSlide, step, 0,
            CLASS_CHANGE_CURTAIN_MAX_SLIDE);
    }

    DrawClassChangeCurtainPanels(screen->curtainSlide);

    if (step > 0) {
        screen->curtainSlide = AddClampedMenuValue(
            screen->curtainSlide, step, 0,
            CLASS_CHANGE_CURTAIN_MAX_SLIDE);
    }

    return screen->curtainSlide;
}
