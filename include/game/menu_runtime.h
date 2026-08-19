#ifndef GAME_MENU_RUNTIME_H
#define GAME_MENU_RUNTIME_H

#include "common.h"
#include "game/menu_types.h"
#include "game/menu_state.h"

typedef enum MenuRuntimePhase {
    MENU_RUNTIME_ACTIVE,
    MENU_RUNTIME_FADING_OUT
} MenuRuntimePhase;

typedef struct MenuRuntime {
    MenuScreenId activeScreen;
    MenuScreenId incomingScreen;
    MenuScreenId outgoingScreen;
    MenuRuntimePhase phase;
    MenuState state;
    MenuVisualState visual;
} MenuRuntime;

typedef enum MenuRuntimeEventType {
    MENU_RUNTIME_EVENT_OPEN,
    MENU_RUNTIME_EVENT_FADE_OUT,
    MENU_RUNTIME_EVENT_ROUTE,
    MENU_RUNTIME_EVENT_RESET
} MenuRuntimeEventType;

typedef struct MenuRuntimeEvent {
    MenuRuntimeEventType type;
    MenuScreenId screen;
    MenuScreenId drawScreen;
} MenuRuntimeEvent;

typedef struct MenuRuntimeResult {
    u8 changed;
} MenuRuntimeResult;

enum MenuRuntimeEffect {
    MENU_RUNTIME_EFFECT_NONE = 0,
    MENU_RUNTIME_EFFECT_MOVE = 1 << 0,
    MENU_RUNTIME_EFFECT_ACCEPT = 1 << 1,
    MENU_RUNTIME_EFFECT_BACK = 1 << 2,
    MENU_RUNTIME_EFFECT_INVALID = 1 << 3,
    MENU_RUNTIME_EFFECT_EXIT = 1 << 4
};

MenuRuntimeResult MenuRuntimeReduce(
    MenuRuntime *runtime, const MenuRuntimeEvent *event);

#endif
