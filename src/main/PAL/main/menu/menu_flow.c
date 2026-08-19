#include "game/audio.h"
#include "game/menu_legacy_globals.h"
#include "game/menu_runtime.h"
#include "game/menu_context.h"
#include "game/menu_state_legacy.h"

void StartMenuExitFade(void);

static MenuRuntime s_MenuRuntime = {
    .activeScreen = MENU_SCREEN_LOADING,
    .incomingScreen = MENU_SCREEN_NONE,
    .outgoingScreen = MENU_SCREEN_NONE,
    .phase = MENU_RUNTIME_FADING_OUT};

static void ApplyMenuRuntime(const MenuRuntime *runtime) {
    g_MenuScreen = runtime->activeScreen;
    g_MenuHandlerIndex = runtime->incomingScreen;
    g_MenuHandlerIndex2 = runtime->outgoingScreen;
}

void MenuFlowOpen(MenuScreenId screen) {
    MenuRuntimeEvent event = {
        MENU_RUNTIME_EVENT_OPEN, screen, MENU_SCREEN_NONE};

    MenuRuntimeReduce(&s_MenuRuntime, &event);
    ApplyMenuRuntime(&s_MenuRuntime);
}

void MenuFlowFadeOut(MenuScreenId screen) {
    MenuRuntimeEvent event = {
        MENU_RUNTIME_EVENT_FADE_OUT, screen, MENU_SCREEN_NONE};

    MenuRuntimeReduce(&s_MenuRuntime, &event);
    ApplyMenuRuntime(&s_MenuRuntime);
}

void MenuFlowRoute(MenuScreenId updateScreen, MenuScreenId drawScreen) {
    MenuRuntimeEvent event = {
        MENU_RUNTIME_EVENT_ROUTE, updateScreen, drawScreen};

    MenuRuntimeReduce(&s_MenuRuntime, &event);
    ApplyMenuRuntime(&s_MenuRuntime);
}

void MenuFlowReset(void) {
    MenuRuntimeEvent event = {
        MENU_RUNTIME_EVENT_RESET, MENU_SCREEN_LOADING, MENU_SCREEN_NONE};

    MenuRuntimeReduce(&s_MenuRuntime, &event);
    ApplyMenuRuntime(&s_MenuRuntime);
}

void MenuFlowSetNavigation(const MenuRuntime *runtime) {
    s_MenuRuntime.activeScreen = runtime->activeScreen;
    s_MenuRuntime.incomingScreen = runtime->incomingScreen;
    s_MenuRuntime.outgoingScreen = runtime->outgoingScreen;
    s_MenuRuntime.phase = runtime->phase;
    ApplyMenuRuntime(&s_MenuRuntime);
}

const MenuRuntime *MenuFlowGetRuntime(void) {
    return &s_MenuRuntime;
}

MenuState *MenuFlowGetState(void) {
    return &s_MenuRuntime.state;
}

MenuVisualState *MenuFlowGetVisualState(void) {
    return &s_MenuRuntime.visual;
}

void MenuFlowInitializeState(
    const MenuState *state, const MenuVisualState *visual) {
    s_MenuRuntime.state = *state;
    s_MenuRuntime.visual = *visual;
    MenuFlowPublishLegacyState();
}

void MenuFlowPublishLegacyState(void) {
    MenuStateApplyLegacy(&s_MenuRuntime.state);
    MenuVisualStateApplyLegacy(&s_MenuRuntime.visual);
}

void MenuFlowApplyEffects(unsigned int effects) {
    if ((effects & MENU_RUNTIME_EFFECT_MOVE) != 0) PlaySoundCue(1);
    if ((effects & MENU_RUNTIME_EFFECT_ACCEPT) != 0) PlaySoundCue(2);
    if ((effects & MENU_RUNTIME_EFFECT_BACK) != 0) PlaySoundCue(3);
    if ((effects & MENU_RUNTIME_EFFECT_INVALID) != 0) PlaySoundCue(5);
    if ((effects & MENU_RUNTIME_EFFECT_EXIT) != 0) StartMenuExitFade();
}
