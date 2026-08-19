#ifndef GAME_MEMORY_CARD_CONTROLLER_H
#define GAME_MEMORY_CARD_CONTROLLER_H

#include "common.h"

typedef struct SaveSession {
    s32 menuState;
    s32 selection;
    s32 subState;
    s32 cardStatus;
    s32 noCardTicks;
    s32 errorTicks;
    s32 lastMenuState;
    s32 errorPending;
    s32 errorCountdown;
} SaveSession;

typedef enum MemoryCardMenuState {
    MC_MENU_CARD_ERROR = -3,
    MC_MENU_NEW_CARD = -2,
    MC_MENU_NO_CARD = -1,
    MC_MENU_READY = 1,
    MC_MENU_READING_CARD = 2,
    MC_MENU_DETECTING_CARD = 3
} MemoryCardMenuState;

typedef enum MemoryCardPage {
    MC_PAGE_MODE_SELECT = 0,
    MC_PAGE_SLOT_ACTION = 1
} MemoryCardPage;

typedef enum MemoryCardFormatState {
    MC_FORMAT_IDLE = 0,
    MC_FORMAT_CONFIRM = 1,
    MC_FORMAT_PREPARE = 2,
    MC_FORMAT_DELAY = 3,
    MC_FORMAT_EXECUTE = 5,
    MC_FORMAT_SUCCESS_DELAY = 7,
    MC_FORMAT_SUCCESS = 8,
    MC_FORMAT_ERROR = 0xA
} MemoryCardFormatState;

typedef enum MemoryCardReadState {
    MC_READ_WAIT_SCENE = 0,
    MC_READ_WAIT_CARD = 1,
    MC_READ_PREPARE = 2,
    MC_READ_DELAY = 3,
    MC_READ_REFRESH = 5,
    MC_READ_POST_REFRESH = 6,
    MC_READ_SETTLE_PREPARE = 7,
    MC_READ_SETTLE_DELAY = 8,
    MC_READ_COMPLETE = 9
} MemoryCardReadState;

typedef enum MemoryCardNoCardState {
    MC_NO_CARD_PREPARE = 0,
    MC_NO_CARD_DELAY = 1,
    MC_NO_CARD_INPUT = 3
} MemoryCardNoCardState;

typedef enum MemoryCardActionState {
    MC_ACTION_IDLE = 0x00,
    MC_ACTION_CONFIRM_OVERWRITE = 0x0A,
    MC_ACTION_SAVE_PREPARE = 0x0B,
    MC_ACTION_SAVE_DELAY = 0x0C,
    MC_ACTION_SAVE_WRITE = 0x0D,
    MC_ACTION_SAVE_POST_WRITE = 0x0F,
    MC_ACTION_SAVE_REFRESH = 0x10,
    MC_ACTION_SAVE_SETTLE_PREPARE = 0x11,
    MC_ACTION_SAVE_SETTLE_DELAY = 0x12,
    MC_ACTION_SAVE_WAIT_CARD = 0x13,
    MC_ACTION_SAVE_SHOW_RESULT = 0x14,
    MC_ACTION_SAVE_RESULT_DELAY = 0x15,
    MC_ACTION_CARD_FULL = 0x19,
    MC_ACTION_LOAD_PREPARE = 0x1E,
    MC_ACTION_LOAD_INITIAL_DELAY = 0x1F,
    MC_ACTION_LOAD_ACCESS_PREPARE = 0x20,
    MC_ACTION_LOAD_ACCESS_DELAY = 0x21,
    MC_ACTION_LOAD_READ = 0x22,
    MC_ACTION_LOAD_SETTLE_PREPARE = 0x23,
    MC_ACTION_LOAD_SETTLE_DELAY = 0x24,
    MC_ACTION_LOAD_WAIT_CARD = 0x25,
    MC_ACTION_LOAD_SHOW_RESULT = 0x26,
    MC_ACTION_LOAD_RESULT_DELAY = 0x27,
    MC_ACTION_NO_FILE = 0x28
} MemoryCardActionState;

typedef struct MemoryCardActionSession {
    MemoryCardActionState state;
    s32 timer;
    s32 settleTicks;
    s32 busy;
    s32 menuPage;
    s32 menuRowCursor;
    s32 prompt;
} MemoryCardActionSession;

typedef enum MemoryCardActionEffect {
    MC_ACTION_EFFECT_NONE,
    MC_ACTION_EFFECT_WRITE_SLOT,
    MC_ACTION_EFFECT_REFRESH_SLOTS,
    MC_ACTION_EFFECT_LOAD_SLOT,
    MC_ACTION_EFFECT_POLL_CARD
} MemoryCardActionEffect;

typedef enum MemoryCardResultPrompt {
    MC_RESULT_PROMPT_CARD_ERROR = 0x10,
    MC_RESULT_PROMPT_LOAD_OK = 0x11,
    MC_RESULT_PROMPT_SAVE_OK = 0x12
} MemoryCardResultPrompt;

typedef struct MemoryCardCursorResult {
    s32 value;
    u8 moved;
} MemoryCardCursorResult;

s32 MemoryCardControllerShouldPoll(s32 actionBusy, s32 errorPending);
void MemoryCardControllerApplyStatus(SaveSession *state,
                                     s32 cardStatus);
void MemoryCardControllerResolveDetection(SaveSession *state);
void MemoryCardControllerResolveTransition(SaveSession *state);
void MemoryCardActionTick(MemoryCardActionSession *state,
                          s32 finalRowCursor);
void MemoryCardActionObserveCard(MemoryCardActionSession *state,
                                 s32 cardStatus);
void MemoryCardActionShowResult(MemoryCardActionSession *state,
                                s32 saveOperation, s32 succeeded);
MemoryCardActionEffect MemoryCardActionRequestedEffect(
    const MemoryCardActionSession *state);
MemoryCardCursorResult MemoryCardMoveMenuRow(
    s32 value, s32 minimum, s32 maximum, u16 pressedRepeat);
MemoryCardCursorResult MemoryCardSetBinaryChoice(
    s32 value, u16 pressedRepeat);

#endif
