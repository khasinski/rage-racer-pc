#ifndef GAME_STATE_H
#define GAME_STATE_H

extern void (*g_SceneHandlers[])(void);
extern void (*g_PrologueSteps[])(void);

#include "common.h"

#include "game/vector.h"
#include "game/pad.h"
#include "psyq/gte.h"


/* Current top-level game mode; indexes g_GameModeHandlers, dispatched each
 * frame by UpdateOptionScene. */
extern s32 g_GameMode;
#ifdef __psyz
#define g_GameModeHandlers g_NativeGameModeHandlers
#endif
extern void (*g_GameModeHandlers[])(void);

/* MainLoop is the PS-EXE `main` and never returns: init chain, then an
 * endless per-frame loop (CD audio, sequencer, asset loads, the current
 * g_GameModeHandlers entry, VSync, display swap, UpdatePadState). */
void MainLoop(void);
void InitSubsystems(void);


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

extern s32 g_FrameSyncThreshold;
extern s32 g_GameClock;

void RestartMemoryCard(void);

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

void BiosSetMemSize(s32 megabytes);
void DrawBootLogo(void);
void DrawEndingStill(void);
void InitGeom(void);
void InitPad(void *buf0, s32 len0, void *buf1, s32 len1);
void InitRecordTables(void);
void InstallSceneLighting(void);
#ifdef __psyz
s32 ResetGraph(s32 mode);
#else
void ResetGraph(s32 mode);
#endif
void ResetReplayFrameCounts(void);
void StartPad(void);
void StepTrackTextureSwap(void);
void TickSequenceAudio(void);
void __main(void);

extern Matrix g_DefaultColorMatrix;
extern Matrix g_DefaultLightMatrix;

#endif
