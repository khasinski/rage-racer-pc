#include "game/menu.h"

void MenuFlowOpen(enum MenuScreenId screen) {
    g_MenuScreen = screen;
    g_MenuHandlerIndex = screen;
}

void MenuFlowFadeOut(enum MenuScreenId screen) {
    g_MenuHandlerIndex = -1;
    g_MenuHandlerIndex2 = screen;
}
