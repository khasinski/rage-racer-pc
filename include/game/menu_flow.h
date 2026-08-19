#ifndef GAME_MENU_FLOW_H
#define GAME_MENU_FLOW_H

typedef enum MenuScreenId {
    MENU_SCREEN_NONE = -1,
    MENU_SCREEN_LOADING = 0,
    MENU_SCREEN_COURSE_SELECT = 1,
    MENU_SCREEN_RANKING,
    MENU_SCREEN_ENTER_CAR_SELECT,
    MENU_SCREEN_CAR_SELECT,
    MENU_SCREEN_CUSTOMIZE,
    MENU_SCREEN_DESIGN_MODE,
    MENU_SCREEN_TEAM_LOGO,
    MENU_SCREEN_LOGO_SAMPLE,
    MENU_SCREEN_TEAM_NAME,
    MENU_SCREEN_PAINT_COLOR,
    MENU_SCREEN_CAR_SHOP,
    MENU_SCREEN_ENGINEER_SHOP
} MenuScreenId;

void MenuFlowOpen(MenuScreenId screen);
void MenuFlowFadeOut(MenuScreenId screen);
void MenuFlowRoute(MenuScreenId updateScreen, MenuScreenId drawScreen);
void MenuFlowReset(void);
void MenuFlowApplyEffects(unsigned int effects);

#endif
