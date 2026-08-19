#include "game/menu_state.h"

enum { MENU_DEFAULT_VIEW_ANGLE = 500000 };

MenuVisualState MenuVisualStateDefaults(void) {
    const MenuVisualState defaults = {0};
    return defaults;
}

MenuState MenuStateDefaults(s32 courseIndex, u8 *emptyScript) {
    MenuState state = {0};

    state.scripts.courseSelectModal = emptyScript;
    state.scripts.carSelectPopup = emptyScript;
    state.scripts.customizePopup = emptyScript;
    state.scripts.teamLogoSubPanel = emptyScript;
    state.scripts.logoSampleSubPanel = emptyScript;
    state.scripts.carShopModal = emptyScript;
    state.scripts.engineerShopModal = emptyScript;
    state.garage.viewAngle = MENU_DEFAULT_VIEW_ANGLE;
    state.garage.viewAngleTarget = MENU_DEFAULT_VIEW_ANGLE;
    state.garage.pendingCourseIndex = -1;
    state.garage.carSwapToIndex = -1;
    state.garage.courseModelIndex = courseIndex;
    state.transition.hintButtonsVisible = 1;
    return state;
}

void MenuStateReset(MenuState *state, s32 courseIndex, u8 *emptyScript) {
    *state = MenuStateDefaults(courseIndex, emptyScript);
}

void MenuVisualStateReset(MenuVisualState *state) {
    *state = MenuVisualStateDefaults();
}
