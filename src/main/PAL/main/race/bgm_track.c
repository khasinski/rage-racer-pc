#include "game/race_internal.h"

enum {
    FIRST_BGM_CD_TRACK = 3,
    NON_BGM_CD_TRACK = 12,
    REMAPPED_BGM_CD_TRACK = 17,
};

s32 BgmCdTrack(s32 selectedTrack) {
    const s32 cdTrack = selectedTrack + FIRST_BGM_CD_TRACK;
    return cdTrack == NON_BGM_CD_TRACK ? REMAPPED_BGM_CD_TRACK : cdTrack;
}
