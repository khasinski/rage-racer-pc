#ifndef GAME_CD_H
#define GAME_CD_H

#include "common.h"
#include "psyq/cd_types.h"

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

typedef enum CdDriveMode {
    CD_MODE_CDDA = 0x01,
    CD_MODE_AUTO_PAUSE = 0x02,
    CD_MODE_REPORT = 0x04,
} CdDriveMode;

typedef enum CdSyncResult {
    CD_SYNC_PENDING = 0,
    CD_SYNC_COMPLETE = 2,
    CD_SYNC_DISK_ERROR = 5,
} CdSyncResult;

typedef enum CdSyncMode {
    CD_SYNC_WAIT = 0,
    CD_SYNC_POLL = 1,
} CdSyncMode;

typedef struct CdLevels {
    u32 ll;
    u32 lr;
    u32 rr;
    u32 rl;
} CdLevels;

typedef struct Cd {
    s32 restart;
    s32 pendingTrack;
    CdCommandType pendingCommand;
    s32 trackStep;
    s32 commandStep;
    CdlLOC elapsed;
    u8 mode;
    u8 result[8];
    CdLevels mix;
    CdLevels fullMix;
    u8 volume;
    CdlFILE search;
    u8 currentTrack;
    s32 fade;
} Cd;

extern Cd g_Cd;

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

extern s32 g_CdTrackEnded;
/*
 * CD-DA (music) front end. Nothing here talks to the drive directly: each call
 * only posts a request into g_Cd, which TickCdAudio pumps one CdControl at a
 * time (CdlPlay 0x03, CdlPause 0x09, CdlGetlocP 0x11).
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
 * CD-DA attenuator. SetCdVolume applies `volume` (0..0x7F) to the stereo
 * channels and pushes it with CdMix; StartCdVolumeFade sets the remaining frame count of the
 * fade StepCdVolumeFade runs each frame (positive fades out, negative fades
 * back to the targets), clamped to +/-0xFFF.
 */
void SetCdVolume(s32 volume);
void StartCdVolumeFade(s32 frames);
/* Map the 0..15 option-screen level onto the 0..0x7F attenuator. */
void SetCdVolumeSetting(s32 level);

/*
 * The CD-DA pump. TickCdAudio runs once per frame from MainLoop and
 * issues at most one CdControl: a pending track goes to StepCdTrackRequest,
 * otherwise g_Cd.pendingCommand selects play, pause, or resume. Each step
 * function is a small state machine over g_Cd.trackStep / g_Cd.commandStep that
 * clears the pending value when it finishes.
 */
void TickCdAudio(void);
/* Boot-time setup: SPU CD input on, drive into CD-DA mode, track table built,
 * every pending/step word cleared and the volume set to full. */
void InitCdAudio(void);

extern char *g_CdAudioFileNames[];
/* Eight-byte CdlGetlocP response. Retail also names bytes 2 and 3 as
 * g_CdLocMinute/g_CdLocSecond; indexing the shared buffer preserves that
 * overlap on hosts where separately declared globals cannot alias safely. */
void CdMix(u8* vol);

#endif
