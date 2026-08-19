#ifndef GAME_CAR_SELECT_CONTROLLER_H
#define GAME_CAR_SELECT_CONTROLLER_H

#include "common.h"

typedef enum CarSelectState {
    CAR_SELECT_ENGINEER_UNAVAILABLE = -2,
    CAR_SELECT_SHOP_UNAVAILABLE = -1,
    CAR_SELECT_ACTIVE = 0,
    CAR_SELECT_START_RACE = 1,
    CAR_SELECT_TO_CUSTOMIZE = 2,
    CAR_SELECT_TO_CAR_SHOP = 3,
    CAR_SELECT_TO_ENGINEER_SHOP = 4,
    CAR_SELECT_BACK = 5
} CarSelectState;

typedef enum CarSelectCommand {
    CAR_SELECT_COMMAND_NONE,
    CAR_SELECT_COMMAND_START_RACE,
    CAR_SELECT_COMMAND_CUSTOMIZE,
    CAR_SELECT_COMMAND_CAR_SHOP,
    CAR_SELECT_COMMAND_CAR_SHOP_UNAVAILABLE,
    CAR_SELECT_COMMAND_ENGINEER_SHOP,
    CAR_SELECT_COMMAND_ENGINEER_SHOP_UNAVAILABLE,
    CAR_SELECT_COMMAND_BACK
} CarSelectCommand;

typedef struct CarSelectInputResult {
    s32 selection;
    u8 moveCount;
    CarSelectCommand command;
} CarSelectInputResult;

typedef struct CarSelectScreenState {
    CarSelectState phase;
    s32 selection;
} CarSelectScreenState;

typedef struct CarSelectScreenInput {
    u16 pressed;
    s32 grandPrixMode;
    s32 shopCarIndex;
    s32 upgradesAvailable;
    s32 maxClassReached;
    s32 requiredClass;
} CarSelectScreenInput;

typedef struct CarSelectScreenResult {
    CarSelectScreenState state;
    CarSelectCommand command;
    u8 moveCount;
} CarSelectScreenResult;

typedef enum CustomizeCommand {
    CUSTOMIZE_COMMAND_NONE,
    CUSTOMIZE_COMMAND_TIRES,
    CUSTOMIZE_COMMAND_TRANSMISSION,
    CUSTOMIZE_COMMAND_TRANSMISSION_UNAVAILABLE,
    CUSTOMIZE_COMMAND_DESIGN,
    CUSTOMIZE_COMMAND_BACK
} CustomizeCommand;

typedef struct CustomizeInputResult {
    s32 selection;
    u8 moveCount;
    CustomizeCommand command;
} CustomizeInputResult;

typedef enum CustomizeState {
    CUSTOMIZE_CONFIRM_TRANSMISSION = -6,
    CUSTOMIZE_CONFIRM_TIRES = -5,
    CUSTOMIZE_CLOSE_UNAVAILABLE = -4,
    CUSTOMIZE_TRANSMISSION_UNAVAILABLE = -3,
    CUSTOMIZE_TRANSMISSION_PROMPT = -2,
    CUSTOMIZE_TIRE_PROMPT = -1,
    CUSTOMIZE_ACTIVE = 0,
    CUSTOMIZE_TO_DESIGN = 1,
    CUSTOMIZE_TO_CAR_SELECT = 2
} CustomizeState;

typedef enum CustomizeEffect {
    CUSTOMIZE_EFFECT_NONE = 0,
    CUSTOMIZE_EFFECT_ACCEPT = 1 << 0,
    CUSTOMIZE_EFFECT_CANCEL = 1 << 1,
    CUSTOMIZE_EFFECT_APPLY_TRANSMISSION = 1 << 2
} CustomizeEffect;

typedef struct CustomizeScreenState {
    CustomizeState phase;
    s32 selection;
    s32 modalCursor;
    s32 confirmTimer;
} CustomizeScreenState;

typedef struct CustomizeScreenInput {
    u16 pressed;
    s32 grandPrixMode;
    s32 transmissionAvailable;
} CustomizeScreenInput;

typedef struct CustomizeScreenResult {
    CustomizeScreenState state;
    CustomizeCommand command;
    u8 moveCount;
    u8 effects;
} CustomizeScreenResult;

CarSelectInputResult CarSelectHandleInput(s32 selection, s32 grandPrixMode,
                                          s32 shopCarIndex,
                                          s32 upgradesAvailable,
                                          s32 maxClassReached,
                                          s32 requiredClass,
                                          u16 pressed);
CarSelectScreenResult CarSelectReduceInput(
    const CarSelectScreenState *state,
    const CarSelectScreenInput *input);
CustomizeInputResult CustomizeHandleInput(s32 selection, s32 grandPrixMode,
                                          s32 transmissionAvailable,
                                          u16 pressed);
CustomizeScreenResult CustomizeReduceInput(
    const CustomizeScreenState *state,
    const CustomizeScreenInput *input);
CustomizeScreenState CustomizeTickConfirmTimer(
    const CustomizeScreenState *state);
CustomizeScreenState CustomizeFinishPopup(
    const CustomizeScreenState *state);

#endif
