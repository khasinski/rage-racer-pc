#include "game/menu_state.h"

MenuVisualState MenuVisualStateDefaults(void) {
    const MenuVisualState defaults = {0};
    return defaults;
}

MenuState MenuStateDefaults(s32 courseIndex, void *emptyScript) {
    MenuState state = {0};

    state.emptyScript = emptyScript;
    state.viewAngle = 500000;
    state.viewAngleTarget = 500000;
    state.pendingCourseIndex = -1;
    state.carSwapToIndex = -1;
    state.courseModelIndex = courseIndex;
    state.hintButtonsVisible = 1;
    state.visual = MenuVisualStateDefaults();
    return state;
}

void MenuStateReset(MenuState *state, s32 courseIndex, void *emptyScript) {
    *state = MenuStateDefaults(courseIndex, emptyScript);
}

void MenuVisualStateReset(MenuVisualState *state) {
    *state = MenuVisualStateDefaults();
}
