#include "game/audio.h"
#include "game/menu.h"
#include "game/menu_runtime.h"

static MenuRuntime CaptureMenuRuntime(void) {
    MenuRuntime runtime;

    runtime.activeScreen = (MenuScreenId)g_MenuScreen;
    runtime.incomingScreen = (MenuScreenId)g_MenuHandlerIndex;
    runtime.outgoingScreen = (MenuScreenId)g_MenuHandlerIndex2;
    runtime.phase = g_MenuHandlerIndex < 0
        ? MENU_RUNTIME_FADING_OUT : MENU_RUNTIME_ACTIVE;
    return runtime;
}

static void ApplyMenuRuntime(const MenuRuntime *runtime) {
    g_MenuScreen = runtime->activeScreen;
    g_MenuHandlerIndex = runtime->incomingScreen;
    g_MenuHandlerIndex2 = runtime->outgoingScreen;
}

void MenuFlowOpen(MenuScreenId screen) {
    MenuRuntime runtime = CaptureMenuRuntime();
    MenuRuntimeEvent event = {
        MENU_RUNTIME_EVENT_OPEN, screen, MENU_SCREEN_NONE};

    MenuRuntimeReduce(&runtime, &event);
    ApplyMenuRuntime(&runtime);
}

void MenuFlowFadeOut(MenuScreenId screen) {
    MenuRuntime runtime = CaptureMenuRuntime();
    MenuRuntimeEvent event = {
        MENU_RUNTIME_EVENT_FADE_OUT, screen, MENU_SCREEN_NONE};

    MenuRuntimeReduce(&runtime, &event);
    ApplyMenuRuntime(&runtime);
}

void MenuFlowRoute(MenuScreenId updateScreen, MenuScreenId drawScreen) {
    MenuRuntime runtime = CaptureMenuRuntime();
    MenuRuntimeEvent event = {
        MENU_RUNTIME_EVENT_ROUTE, updateScreen, drawScreen};

    MenuRuntimeReduce(&runtime, &event);
    ApplyMenuRuntime(&runtime);
}

void MenuFlowReset(void) {
    MenuRuntime runtime = CaptureMenuRuntime();
    MenuRuntimeEvent event = {
        MENU_RUNTIME_EVENT_RESET, MENU_SCREEN_LOADING, MENU_SCREEN_NONE};

    MenuRuntimeReduce(&runtime, &event);
    ApplyMenuRuntime(&runtime);
}

void MenuFlowApplyEffects(unsigned int effects) {
    if ((effects & MENU_RUNTIME_EFFECT_MOVE) != 0) PlaySoundCue(1);
    if ((effects & MENU_RUNTIME_EFFECT_ACCEPT) != 0) PlaySoundCue(2);
    if ((effects & MENU_RUNTIME_EFFECT_BACK) != 0) PlaySoundCue(3);
    if ((effects & MENU_RUNTIME_EFFECT_INVALID) != 0) PlaySoundCue(5);
    if ((effects & MENU_RUNTIME_EFFECT_EXIT) != 0) StartMenuExitFade();
}
