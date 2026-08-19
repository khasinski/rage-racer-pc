#ifndef GAME_SHOP_CONTROLLER_H
#define GAME_SHOP_CONTROLLER_H

#include "common.h"

typedef enum ShopCommand {
    SHOP_COMMAND_NONE,
    SHOP_COMMAND_OPEN_PURCHASE,
    SHOP_COMMAND_NO_FUNDS,
    SHOP_COMMAND_BACK
} ShopCommand;

typedef struct ShopInputResult {
    s32 selection;
    u8 moveCount;
    ShopCommand command;
} ShopInputResult;

typedef enum ShopScreenPhase {
    SHOP_PHASE_ACTIVE,
    SHOP_PHASE_PURCHASE_PROMPT,
    SHOP_PHASE_NO_FUNDS,
    SHOP_PHASE_COMMITTING,
    SHOP_PHASE_LEAVING,
    SHOP_PHASE_COMPLETED
} ShopScreenPhase;

typedef enum ShopEffect {
    SHOP_EFFECT_NONE = 0,
    SHOP_EFFECT_ACCEPT = 1 << 0,
    SHOP_EFFECT_CANCEL = 1 << 1,
    SHOP_EFFECT_BEGIN_COMMIT = 1 << 2
} ShopEffect;

typedef struct ShopScreenState {
    ShopScreenPhase phase;
    s32 selection;
    s32 modalCursor;
    s32 confirmTimer;
} ShopScreenState;

typedef struct ShopScreenInput {
    u16 pressed;
    s32 canOpenPurchase;
    s32 showNoFundsWhenBlocked;
    s32 hasFunds;
} ShopScreenInput;

typedef struct ShopScreenResult {
    ShopScreenState state;
    ShopCommand command;
    u8 moveCount;
    u8 effects;
} ShopScreenResult;

ShopInputResult ShopHandleInput(s32 selection, s32 canOpenPurchase,
                                s32 showNoFundsWhenBlocked, u16 pressed);
ShopScreenResult ShopReduceInput(const ShopScreenState *state,
                                 const ShopScreenInput *input);
ShopScreenState ShopTickConfirmTimer(const ShopScreenState *state);
ShopScreenState ShopFinishCommit(const ShopScreenState *state);

#endif
