#ifndef GAME_CD_INTERNAL_H
#define GAME_CD_INTERNAL_H

#include <sys/types.h>

#include "common.h"
#include "psyq/cd_types.h"

extern CdlLOC g_CdTrackLocs[18];
extern CdlLOC *g_CdBgmTrackLocs;
extern CdlLOC g_CdTrackLoopPoint[18];
extern u32 g_CdMixLL;
extern u32 g_CdMixLR;
extern u32 g_CdMixRR;
extern u32 g_CdMixRL;
extern u32 g_CdMixFullLL;
extern u32 g_CdMixFullLR;
extern u32 g_CdMixFullRR;
extern u32 g_CdMixFullRL;

#endif
