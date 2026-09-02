#ifndef GAME_CD_H
#define GAME_CD_H

#include "common.h"

struct CdlLOC;
struct CdlFILE;

typedef enum CdCommandType {
    CD_COMMAND_NONE = -1,
    CD_COMMAND_PLAY = 1,
    CD_COMMAND_PAUSE,
    CD_COMMAND_RESUME
} CdCommandType;

typedef enum CdDriveCommand {
    CD_DRIVE_PLAY = 0x03,
    CD_DRIVE_PAUSE = 0x09,
    CD_DRIVE_SET_MODE = 0x0E,
    CD_DRIVE_GET_LOCATION = 0x11,
    CD_DRIVE_SEEK_PLAY = 0x16,
} CdDriveCommand;

typedef enum CdSyncResult {
    CD_SYNC_PENDING = 0,
    CD_SYNC_COMPLETE = 2,
    CD_SYNC_DISK_ERROR = 5,
} CdSyncResult;

typedef enum CdSyncMode {
    CD_SYNC_WAIT = 0,
    CD_SYNC_POLL = 1,
} CdSyncMode;

/* An asserted host EOF may be consumed only once the previous request has
 * finished. The backend keeps EOF asserted until the following Play. */
static inline int CdAudioRequestsIdle(
    s32 trackPending, CdCommandType commandPending) {
    return trackPending < 0 && commandPending == CD_COMMAND_NONE;
}

static inline int CdTrackHasLoopPoint(s32 firstLoopPoint, s32 loopPoint) {
    return firstLoopPoint < loopPoint;
}

static inline int CdPlaybackPassedLoopPoint(
    s32 firstLoopPoint, s32 loopPoint, s32 elapsed) {
    return CdTrackHasLoopPoint(firstLoopPoint, loopPoint) &&
           elapsed >= loopPoint;
}

extern u8 g_CdVolume;
/*
 * CD-DA (music) front end. Nothing here talks to the drive directly: each call
 * only posts a request into g_CdTrackPending..g_CdCommandStep, which TickCdAudio pumps
 * one CdControl at a time (CdlPlay 0x03, CdlPause 0x09, CdlGetlocP 0x11).
 */
/* Queue track `track` from the g_CdTrackLocs CdlLOC table. */
void RequestCdTrack(s32 track);
/* Issue CdlPlay for whatever is queued / paused. */
void StartCdAudio(void);
/* Capture the current position, then CdlPause. */
void PauseCdAudio(void);
/* Undo the pause: replay the track when the pause crossed a track boundary. */
void ResumeCdAudio(void);
/* Drop any pending track/command and reset the current track index to 2. */
void ResetCdAudioState(void);

/*
 * CD-DA attenuator. SetCdVolume scales the four g_CdMixPresets mix values by
 * `volume` (0..0x7F) into both the current and target levels and pushes them
 * with CdMix; StartCdVolumeFade sets the remaining frame count of the
 * fade StepCdVolumeFade runs each frame (positive fades out, negative fades
 * back to the targets), clamped to +/-0xFFF.
 */
void SetCdVolume(s32 volume);
void StartCdVolumeFade(s32 frames);
void StepCdVolumeFade(void);
/* Re-push the current g_CdVolume (used after a mode change). */
/* Map the 0..15 option-screen level onto the 0..0x7F attenuator. */
void SetCdVolumeSetting(s32 level);
/* Select which 4-byte row of the g_CdMixPresets mix table SetCdVolume scales. */
void SetCdMixPreset(s32 preset);

/*
 * The CD-DA pump. TickCdAudio runs once per frame from MainLoop and
 * issues at most one CdControl: a pending track goes to StepCdTrackRequest,
 * otherwise g_CdCommandPending selects play, pause, or resume. Each step
 * function is a small state machine over g_CdTrackStep / g_CdCommandStep that
 * clears the pending value when it finishes.
 */
void TickCdAudio(void);
void StepCdTrackRequest(void);   /* CdlSeekP 0x16 */
/* CdlPlay 0x03, for both a fresh play and a resume: the sequence is the
 * same, so the dispatch sends both commands here. */
void StepCdPlayRequest(void);
void StepCdPauseRequest(void);   /* CdlGetlocP + CdlPause */
/* Boot-time setup: SPU CD input on, drive into CD-DA mode, track table built,
 * every pending/step word cleared and the volume set to full. */
void InitCdAudio(void);
/* Build the g_CdTrackLocs CdlLOC table: CdGetToc, each audio track pushed 0x3C
 * sectors in, then the file-backed entries found with DsSearchFile. */
void BuildCdTrackTable(void);

/* Declared identically by 27 translation units before this
 * header carried them. */

extern CdCommandType g_CdCommandPending;
extern s32 g_CdCommandStep;
extern u8 g_CdCurrentTrack;
extern s32 g_CdFadeFrames;
extern s32 g_CdMixPreset;
extern s32 g_CdRestartOnResume;
extern s32 g_CdTrackPending;
extern s32 g_CdTrackStep;

/* Declared identically by 10 translation units before this
 * header carried them. */

extern char *g_CdAudioFileNames[];
/* Eight-byte CdlGetlocP response. Retail also names bytes 2 and 3 as
 * g_CdLocMinute/g_CdLocSecond; indexing the shared buffer preserves that
 * overlap on hosts where separately declared globals cannot alias safely. */
extern u8 g_CdLocResult[8];
extern u8 g_CdMixPresets[];
extern u8 g_CdModeParam;
extern struct CdlFILE g_CdSearchFile;
extern s32 g_CdTocEntryCount;
extern struct CdlLOC g_CdTrackElapsedLoc;

void CdMix(u8* vol);

#endif
