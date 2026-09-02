#ifndef GAME_STATE_H
#define GAME_STATE_H

extern void (*g_SceneHandlers[40])(void);

#include "common.h"

extern u8 g_PadType;
#include "game/vector.h"
#include "psyq/gte.h"

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

enum PadType {
    PAD_TYPE_NEGCON = 0x23,
    PAD_TYPE_DIGITAL = 0x41
};

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

/* Current top-level game mode; indexes g_GameModeHandlers, dispatched each
 * frame by UpdateOptionScene. */
extern s32 g_GameMode;
#define g_GameModeHandlers g_NativeGameModeHandlers
extern void (*g_GameModeHandlers[])(void);

/* Boot the game and run frames until the host requests shutdown. */
void MainLoop(void);
void InitSubsystems(void);

/* Controller layer. GameInitPad hands the BIOS the two 0x28-byte halves of
 * g_PadBuffers. UpdatePadState maintains the held / previous / newly-pressed
 * halfwords in the block at g_PadState. */
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

/* Identity of the running scene: queried (`== 0xC`, `== 0x11`, `== 0x1E`, ...)
 * but never dispatched. Every writer also resets g_SceneTimer. */
extern s32 g_SceneId;

/* Per-scene frame counter, reset with every g_SceneId write. Scenes sequence
 * themselves against fixed thresholds. Four TUs need it as u32 and carry their
 * own unsigned declaration of the same symbol. */
extern s32 g_SceneTimer;

/* Free-running animation phase counter: drives cyclic effects (sine offsets,
 * blink tests `& 2` / `& 8`, `% 6` cycles), never a deadline. */
extern s32 g_AnimTimer;

/*
 * Boot-time defaults for everything the memory card persists: the three car
 * tables, the three GameRaceProgress slots, both course-progress blocks,
 * g_MaxClassReached, the BGM selection and the three audio settings. Called
 * once, from InitSubsystems.
 */
void InitSaveDefaults(void);
/* Reset the current g_CourseProgress block (class < 2 marks slot 3 unused). */
void ResetCourseProgress(s32 classIndex);

extern s32 g_PadErrorHoldBits;
extern s32 g_PadValidateCountdown;
extern s32 g_FrameSyncThreshold;
extern s32 g_GameClock;
extern s32 g_OptionLetterboxHeight;
typedef enum PadErrorState {
    PAD_ERROR_STATE_INVALID = -1,
    PAD_ERROR_STATE_NONE,
    PAD_ERROR_STATE_DISCONNECTED,
    PAD_ERROR_STATE_INVALID_INPUT
} PadErrorState;

extern PadErrorState g_PadErrorState;


extern u16 g_NegconSteerDeadZone[][2];
extern s32 g_BootLogoHoldTimer;
typedef enum BootLogoState {
    BOOT_LOGO_STATE_INVALID = -1,
    BOOT_LOGO_STATE_FADE_IN,
    BOOT_LOGO_STATE_HOLD,
    BOOT_LOGO_STATE_FADE_OUT,
    BOOT_LOGO_STATE_START_FMV
} BootLogoState;

extern BootLogoState g_BootLogoState;
extern s32 g_BootLogoTimer;
extern s32 g_FrameCounter;
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
extern u16 g_PadHeld;
extern u16 g_PadPressedRepeat;
extern u16 g_PadPressed;

extern u8 g_PadRepeatTimer[];
extern s16 g_NegconAnalogI;
extern s16 g_NegconAnalogII;
extern s16 g_NegconAnalogL;
extern s16 g_NegconSteer;

void BiosSetMemSize(s32 megabytes);
void DrawBootLogo(void);
void UpdateBootLogoScene(void);
void DrawEndingStill(void);
void UpdateEndingStill(void);
void InitGeom(void);
void InitPad(void *buf0, s32 len0, void *buf1, s32 len1);
void InitRecordTables(void);
s32 ResetGraph(s32 mode);
void StepTrackTextureSwap(void);

extern Matrix g_DefaultColorMatrix;
extern Matrix g_DefaultLightMatrix;
extern DVec g_PadCalloutButtonPoints[];
extern DVec g_PadCalloutLabelPoints[];
extern DVec g_PadLabelSlots[];

#endif
