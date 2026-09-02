#ifndef GAME_CD_INTERNAL_H
#define GAME_CD_INTERNAL_H

#include <sys/types.h>

#include "common.h"
#include "psyq/cd_types.h"

typedef enum CdTrackRequestStep {
    CD_TRACK_WAIT_FOR_DRIVE,
    CD_TRACK_SEND_SEEK,
    CD_TRACK_WAIT_FOR_SEEK,
    CD_TRACK_FINISH_SELECTION,
    CD_TRACK_RESTART_WAIT_FOR_DRIVE,
    CD_TRACK_RESTART_SEND_SEEK,
    CD_TRACK_RESTART_WAIT_FOR_SEEK,
    CD_TRACK_FINISH_RESTART,
    CD_TRACK_WAIT_FOR_PAUSE,
} CdTrackRequestStep;

typedef enum CdPlayRequestStep {
    CD_PLAY_WAIT_FOR_DRIVE,
    CD_PLAY_SEND_COMMAND,
    CD_PLAY_WAIT_FOR_COMMAND,
    CD_PLAY_FINISH,
} CdPlayRequestStep;

enum {
    CD_TRACK_LOCATION_COUNT = 18,
    CD_FILE_TRACK_COUNT = 16,
    CD_TOC_CAPACITY = 100,
};

extern CdlLOC g_CdTrackLocs[CD_TRACK_LOCATION_COUNT];
extern CdlLOC *g_CdBgmTrackLocs;
extern CdlLOC g_CdTrackLoopPoint[CD_TRACK_LOCATION_COUNT];
extern u32 g_CdMixLL;
extern u32 g_CdMixLR;
extern u32 g_CdMixRR;
extern u32 g_CdMixRL;
extern u32 g_CdMixFullLL;
extern u32 g_CdMixFullLR;
extern u32 g_CdMixFullRR;
extern u32 g_CdMixFullRL;

#endif
