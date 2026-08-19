#ifndef GAME_MENU_FLOW_H
#define GAME_MENU_FLOW_H

#include "game/menu_runtime.h"

void MenuFlowOpen(MenuScreenId screen);
void MenuFlowFadeOut(MenuScreenId screen);
void MenuFlowRoute(MenuScreenId updateScreen, MenuScreenId drawScreen);
void MenuFlowReset(void);
void MenuFlowSetNavigation(const MenuRuntime *runtime);
const MenuRuntime *MenuFlowGetRuntime(void);
void MenuFlowApplyEffects(unsigned int effects);

#endif
