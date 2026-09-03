#include "game/audio_internal.h"
#include "game/integer.h"
#include "game/race_internal.h"

#include <stddef.h>

enum {
    FIRST_BGM_CD_TRACK = 3,
    NON_BGM_CD_TRACK = 12,
    REMAPPED_BGM_CD_TRACK = 17,
};

s32 BgmCdTrack(s32 selectedTrack) {
    const s32 cdTrack = WrapSigned32(
        (int64_t)selectedTrack + FIRST_BGM_CD_TRACK);
    return cdTrack == NON_BGM_CD_TRACK ? REMAPPED_BGM_CD_TRACK : cdTrack;
}

s32 WrapBgmTrackIndex(s32 track, s32 trackCount) {
    if (trackCount <= 0) {
        return 0;
    }

    track %= trackCount;
    return track < 0 ? track + trackCount : track;
}

s32 ClampBgmTrackCount(s32 trackCount) {
    if (trackCount < 0) {
        return 0;
    }
    return trackCount < BGM_PLAYABLE_TRACK_COUNT
               ? trackCount
               : BGM_PLAYABLE_TRACK_COUNT;
}

s32 BgmShuffleTrackAt(const u8 *shuffleOrder, s32 trackCount,
                      s32 shuffleIndex) {
    s32 track;

    trackCount = ClampBgmTrackCount(trackCount);
    if (shuffleOrder == NULL || trackCount == 0) {
        return 0;
    }
    shuffleIndex = WrapBgmTrackIndex(shuffleIndex, trackCount);
    track = shuffleOrder[shuffleIndex];
    return track < trackCount ? track : 0;
}
