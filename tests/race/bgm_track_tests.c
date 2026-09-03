#include <assert.h>
#include <limits.h>
#include <stddef.h>

#include "game/audio_internal.h"
#include "game/race_internal.h"

int main(void) {
    const u8 order[] = {2, 0, 1, 3, 4, 5, 6, 7, 8, 9};

    assert(BgmCdTrack(0) == 3);
    assert(BgmCdTrack(8) == 11);
    assert(BgmCdTrack(9) == 17);
    assert(BgmCdTrack(10) == 13);
    assert(BgmCdTrack(INT_MAX) == INT_MIN + 2);
    assert(BgmCdTrack(INT_MIN) == INT_MIN + 3);
    assert(WrapBgmTrackIndex(-1, 10) == 9);
    assert(WrapBgmTrackIndex(0, 10) == 0);
    assert(WrapBgmTrackIndex(10, 10) == 0);
    assert(WrapBgmTrackIndex(21, 10) == 1);
    assert(WrapBgmTrackIndex(3, 0) == 0);
    assert(ClampBgmTrackCount(-1) == 0);
    assert(ClampBgmTrackCount(9) == 9);
    assert(ClampBgmTrackCount(INT_MAX) == BGM_PLAYABLE_TRACK_COUNT);
    assert(BgmShuffleTrackAt(order, 3, 0) == 2);
    assert(BgmShuffleTrackAt(order, 3, 4) == 0);
    assert(BgmShuffleTrackAt(order, 3, -1) == 1);
    assert(BgmShuffleTrackAt(order, 3, INT_MIN) == 0);
    assert(BgmShuffleTrackAt(order, 0, 0) == 0);
    assert(BgmShuffleTrackAt(NULL, 3, 0) == 0);
    assert(BgmShuffleTrackAt(order, INT_MAX, 9) == 9);
    return 0;
}
