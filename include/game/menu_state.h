#ifndef GAME_MENU_STATE_H
#define GAME_MENU_STATE_H

#include "common.h"

typedef struct MenuVisualState {
    s32 courseSelect;
    s32 ranking;
    s32 carSelect;
    s32 customize;
    s32 designMode;
    s32 teamLogo;
    s32 logoSample;
    s32 teamName;
    s32 paintColor;
    s32 carShop;
    u32 engineerShop;
    s32 carSpecGraph;
    s32 lightBurst;
    s32 timeAttackPlate;
} MenuVisualState;

typedef struct MenuScriptState {
    u8 *courseSelectModal;
    u8 *carSelectPopup;
    u8 *customizePopup;
    void *teamLogoSubPanel;
    void *logoSampleSubPanel;
    u8 *carShopModal;
    u8 *engineerShopModal;
} MenuScriptState;

typedef struct GaragePresentationState {
    s32 viewAngle;
    s32 viewAngleTarget;
    s32 viewOffset;
    s32 viewOffsetTarget;
    s32 courseCardSpin;
    s32 courseCardSpinTarget;
    s32 courseCardPendingGrade;
    s32 pendingCourseIndex;
    s32 carSwapFromIndex;
    s32 carSwapToIndex;
    s32 overlayPattern;
    s32 carNamePlateStep;
    s32 plateCarIndex;
    s32 carSpecGraphStep;
    s32 courseModelIndex;
} GaragePresentationState;

typedef struct MenuTransitionState {
    s32 uiScriptProgress;
    s32 uiScriptProgress2;
    s32 hintBarProgress;
    s32 confirmTimer;
    s32 busy;
    s32 hintBarStep;
    s32 classChangeApplied;
    s32 courseSwapDelay;
    s32 altPanelStep;
    s32 altPanelStep2;
    s32 timeAttackPlateStep;
    s32 hintButtonsVisible;
} MenuTransitionState;

typedef struct MenuSelectionState {
    s32 altLayoutSetting;
    s32 carShopUnlockAll;
    s32 courseSelectOption;
    s32 carSelectCursor;
    s32 rankingOption;
    s32 designModeOption;
} MenuSelectionState;

typedef struct MenuState {
    MenuScriptState scripts;
    GaragePresentationState garage;
    MenuTransitionState transition;
    MenuSelectionState selection;
} MenuState;

MenuState MenuStateDefaults(s32 courseIndex, u8 *emptyScript);
MenuVisualState MenuVisualStateDefaults(void);
void MenuStateReset(MenuState *state, s32 courseIndex, u8 *emptyScript);
void MenuVisualStateReset(MenuVisualState *state);

#endif
