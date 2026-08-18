#ifndef GAME_PAD_H
#define GAME_PAD_H

#include "common.h"
#include "game/vector.h"

extern u8 g_PadType;
/*
 * The pad block at 0x801E4368, filled by UpdatePadState from the raw BIOS
 * buffer at g_PadBuffers. `held` is the inverted button halfword; the two
 * edge words differ only in auto-repeat, which UpdatePadState folds into
 * `pressedRepeat` once a button has been held for 30 frames.
 *
 * Three of the fields are also declared as standalone globals, which is where
 * their names come from: held is g_PadHeld, pressed is g_PadPressed and
 * pressedRepeat is g_PadPressedRepeat.
 */
typedef struct PadState {
    u8 status;
    u8 type; /* 0x41 digital pad, 0x23 NeGcon */
    u16 held;
    u16 prevHeld;
    s16 pressed;
    s16 pressedRepeat;
    s16 twist;   /* NeGcon twist axis, 0x80 centred */
    s16 buttonI;
    s16 buttonII;
    s16 buttonL;
    s16 unk12;
    s16 unk14;
    s16 steer;   /* twist after the neutral offset, play deadzone and clamp */
} PadState;

/*
 * Button bits after UpdatePadState has inverted the BIOS packet. CONFIRM and
 * CANCEL are the composites the menus test: any of start, cross or circle
 * confirms, either of square or triangle backs out.
 */
enum PadButton {
    PAD_L2 = 0x1,
    PAD_R2 = 0x2,
    PAD_L1 = 0x4,
    PAD_R1 = 0x8,
    PAD_TRIANGLE = 0x10,
    PAD_CIRCLE = 0x20,
    PAD_CROSS = 0x40,
    PAD_SQUARE = 0x80,
    PAD_SELECT = 0x100,
    PAD_L3 = 0x200,
    PAD_R3 = 0x400,
    PAD_START = 0x800,
    PAD_UP = 0x1000,
    PAD_RIGHT = 0x2000,
    PAD_DOWN = 0x4000,
    PAD_LEFT = 0x8000,

    PAD_CONFIRM = PAD_START | PAD_CROSS | PAD_CIRCLE,
    PAD_CANCEL = PAD_SQUARE | PAD_TRIANGLE,
    PAD_DPAD = PAD_UP | PAD_DOWN | PAD_LEFT | PAD_RIGHT
};
/* Controller layer. GameInitPad hands the BIOS the two 0x28-byte buffers at
 * g_PadBuffers / g_Pad2Buffer. UpdatePadState maintains the held / previous /
 * newly-pressed halfwords in the block at g_PadState. */
void GameInitPad(void);
void UpdatePadState(void);
void LoadPadButtonMapping(s32 mapping0, s32 mapping1);
void ApplyPadButtonMapping(void);
extern PadState g_PadState;

/* Controller-config and NeGcon calibration screens: g_GameModeHandlers entries
 * 7..11, each drawing its own screen plus the shared 3D backdrop. */
void UpdateControllerConfigScreen(void);
void DrawControllerConfigScreen(void);
void BeginNegconCalibration(void);
void UpdateNegconNeutralScreen(void);
void DrawNegconNeutralScreen(void);
void UpdateNegconSteerPlayScreen(void);
void DrawNegconSteerPlayScreen(void);
void UpdateNegconMaxTwistScreen(void);
void DrawNegconMaxTwistScreen(void);
void DrawControllerSetupScene(s32 variant);

/*
 * Controller-configuration screen widgets. Two independent 0..7 selections:
 * g_PadMappingIndex for the standard pad, g_NegconMappingIndex for the NeGcon (pad type byte
 * g_PadType == 0x23 picks which diagram is drawn).
 */
/* 16x32 arrow sprites at (0x28, 0xE0) and (0x108, 0xE0); `pulse` adds the glow. */
u8 *DrawLeftArrow(void *ot, u8 *prim, s32 x, s32 y, s32 pulse);
u8 *DrawRightArrow(void *ot, u8 *prim, s32 x, s32 y, s32 pulse);
/* Framed panel showing the selected configuration number. */
u8 *DrawPadConfigSelector(void *ot, u8 *prim, s32 x, s32 y, s32 selection);
/* The five action labels, and the five lines from each label to its button. */
u8 *DrawPadConfigLabels(void *ot, u8 *prim, u8 *labelRow);
u8 *DrawPadConfigCallouts(void *ot, u8 *prim, u8 *labelRow, u8 *buttonRow);
/* One whole controller diagram for the current selection: labels + callouts. */
u8 *DrawPadConfigDiagram(void *ot, u8 *prim);
u8 *DrawNegconConfigDiagram(void *ot, u8 *prim);
/* Entry hook: backs both selections up to g_PadMappingIndexSaved / g_NegconMappingIndexSaved so a cancel
 * can restore them. Its caller sets g_GameMode = 7 in the same breath. */
void BeginControllerConfig(void);
/* Declared identically by 42 translation units before this
 * header carried them. */

extern s32 g_PadErrorHoldBits;
extern s32 g_PadValidateCountdown;
extern s32 g_ControllerSceneAngleX;
extern s32 g_ControllerSceneAngleY;
extern u16 g_NegconMappingIndexSaved;
extern s32 g_OptionLetterboxHeight;
extern s32 g_PadConfigFlipPhase;
extern s32 g_PadConfigFlipTimer;
typedef enum PadErrorState {
    PAD_ERROR_STATE_INVALID = -1,
    PAD_ERROR_STATE_NONE,
    PAD_ERROR_STATE_DISCONNECTED,
    PAD_ERROR_STATE_INVALID_INPUT
} PadErrorState;

extern PadErrorState g_PadErrorState;
extern u16 g_PadMappingIndexSaved;
/* Declared identically by 38 translation units before this
 * header carried them. */

extern u16 g_NegconSteerDeadZone[][2];
extern u16 g_NegconButtonPresets[];
extern u8 g_NegconConfigButtonRows[];
extern u8 g_NegconConfigLabelRows[];
extern u8 g_PadBuffers[0x50];
#define g_PadBufferType g_PadBuffers[1]
#define g_PadBufferButtonsHigh g_PadBuffers[2]
#define g_PadBufferButtonsLow g_PadBuffers[3]
extern u16 g_PadButtonPresets[];
extern u8 g_PadConfigButtonRows[];
extern u8 g_PadConfigLabelRows[];
extern u16 g_PadPrevHeld;
extern volatile u16 g_PadHeld;

typedef union PadHeldAddress {
    volatile u16 *live;
    const u16 *stable;
} PadHeldAddress;

static inline u16 ReadStablePadHeld(void) {
    PadHeldAddress address;

    address.live = &g_PadHeld;
    return *address.stable;
}
extern u16 g_PadPressedRepeat;
extern u16 g_PadPressed;

typedef struct PadPressedView {
    u16 buttons;
} PadPressedView;

typedef union PadPressedAddress {
    u16 *buttons;
    PadPressedView *view;
} PadPressedAddress;

static inline PadPressedView *GetPadPressedView(void) {
    PadPressedAddress address;

    address.buttons = &g_PadPressed;
    return address.view;
}

extern u8 g_PadRepeatTimer[];
extern s16 g_NegconAnalogI;
extern s16 g_NegconAnalogII;
extern s16 g_NegconAnalogL;
extern s16 g_NegconSteer;
/* Declared identically by 5 translation units before this
 * header carried them. */

extern DVec g_PadCalloutButtonPoints[];
extern DVec g_PadCalloutLabelPoints[];
extern DVec g_PadLabelSlots[];

#endif
