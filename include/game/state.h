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

/* MainLoop is the PS-EXE `main` and never returns: init chain, then an
 * endless per-frame loop (CD audio, sequencer, asset loads, the current
 * g_GameModeHandlers entry, VSync, display swap, UpdatePadState). */
void MainLoop(void);
void InitSubsystems(void);

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
 * FMV playback ("\RAGE.STR;1" streams). One of the three GameBegin*Fmv wrappers
 * picks the stream entry in g_StreamCdEntries, records the scene to come back to
 * in g_StreamReturnScene and sets g_SceneId = 5; from then on UpdateFmv runs
 * per frame and walks g_FmvState through 0 (start) -> 1 (decode) -> 2 (finish).
 * Start (pad bit 0x800) or the end of the stream both move it to 2.
 * The per-TU-typed members of the family - StartFmvPlayback,
 * SetupFmvBuffers, InitFmvContext, OpenFmvStream,
 * PresentFmvFrame, WaitFmvDecode, StartStreamRead - keep their
 * aliased declarations in each file.
 */
/* Start one of the three streams; returnScene is the g_SceneId to come back
 * to when it ends. Each is a thin wrapper that forwards its argument to
 * BeginFmv and then picks the g_StreamCdEntries entry. */
/* Shared prologue of the three: close the audio slots, park g_SceneId at 5
 * and record returnScene in g_StreamReturnScene. */
void BeginFmv(s32 returnScene);
void BeginIntroFmv(s32 returnScene);
void BeginClassFmv(s32 returnScene);
void BeginEndingFmv(s32 returnScene);
void UpdateFmv(void);
/* One decoded frame: DecDCTin the next bitstream chunk, DecDCTout the previous
 * one, then top the ring up from the drive. */
void DecodeFmvFrame(void);
/* Clear the DecDCTout callback, unhook the streamer, restore g_SceneId. */
void EndFmv(void);
/* Pull the next ready ring frame and resize the display when the stream's
 * frame size changes; returns 0 when nothing is ready. */
/* The DMA1 (MDECout) callback: LoadImage one decoded strip into VRAM and queue
 * the next strip, or flip to the other frame buffer at the end of a frame. */
void UploadFmvSlice(void);

/*
 * Boot-time defaults for everything the memory card persists: the three car
 * tables, the three GameRaceProgress slots, both course-progress blocks,
 * g_MaxClassReached, the BGM selection and the three audio settings. Called
 * once, from InitSubsystems.
 */
void InitSaveDefaults(void);
/* Reset the current g_CourseProgress block (arg < 2 also marks slot 3 free). */
void ResetCourseProgress(s32 mode);

/* Declared identically by 42 translation units before this
 * header carried them. */

extern s32 g_PadErrorHoldBits;
extern s32 g_PadValidateCountdown;
extern s32 g_ControllerSceneAngleX;
extern s32 g_ControllerSceneAngleY;
extern s32 g_FrameSyncThreshold;
extern s32 g_GameClock;
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
extern u8 g_LibcLowerDigits[];
extern u8 g_LibcNullText[];
extern s32 g_LibcOutColumn;
extern u8 g_LibcUpperDigits[];
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
void ResetReplayFrameCounts(void);
void StartPad(void);
void StepTrackTextureSwap(void);

/* Declared identically by 5 translation units before this
 * header carried them. */

extern Matrix g_DefaultColorMatrix;
extern Matrix g_DefaultLightMatrix;
extern DVec g_PadCalloutButtonPoints[];
extern DVec g_PadCalloutLabelPoints[];
extern DVec g_PadLabelSlots[];

#endif
