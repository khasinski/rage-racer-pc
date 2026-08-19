#ifndef GAME_MENU_STATE_LEGACY_H
#define GAME_MENU_STATE_LEGACY_H

#include "game/menu_state.h"

/* Compatibility boundary for the original absolute menu globals. */
void MenuStateApplyLegacy(const MenuState *state);
void MenuStateCaptureLegacy(MenuState *state);
void MenuVisualStateApplyLegacy(const MenuVisualState *state);
void MenuVisualStateCaptureLegacy(MenuVisualState *state);
void MenuVisualStateResetLegacy(void);

#endif
