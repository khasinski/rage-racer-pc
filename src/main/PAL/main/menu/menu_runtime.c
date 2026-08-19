#include "game/menu_runtime.h"

MenuRuntimeResult MenuRuntimeReduce(
    MenuRuntime *runtime, const MenuRuntimeEvent *event) {
    MenuRuntime previous = *runtime;
    MenuRuntimeResult result;

    switch (event->type) {
    case MENU_RUNTIME_EVENT_OPEN:
        runtime->activeScreen = event->screen;
        runtime->incomingScreen = event->screen;
        runtime->outgoingScreen = event->screen;
        runtime->phase = MENU_RUNTIME_ACTIVE;
        break;
    case MENU_RUNTIME_EVENT_FADE_OUT:
        runtime->incomingScreen = MENU_SCREEN_NONE;
        runtime->outgoingScreen = event->screen;
        runtime->phase = MENU_RUNTIME_FADING_OUT;
        break;
    case MENU_RUNTIME_EVENT_ROUTE:
        runtime->activeScreen = event->screen;
        runtime->incomingScreen = event->drawScreen;
        runtime->phase = MENU_RUNTIME_ACTIVE;
        break;
    case MENU_RUNTIME_EVENT_RESET:
        runtime->activeScreen = MENU_SCREEN_LOADING;
        runtime->incomingScreen = MENU_SCREEN_NONE;
        runtime->outgoingScreen = MENU_SCREEN_NONE;
        runtime->phase = MENU_RUNTIME_FADING_OUT;
        break;
    }
    result.changed =
        runtime->activeScreen != previous.activeScreen ||
        runtime->incomingScreen != previous.incomingScreen ||
        runtime->outgoingScreen != previous.outgoingScreen ||
        runtime->phase != previous.phase;
    return result;
}
