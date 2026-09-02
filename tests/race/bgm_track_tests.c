#include <assert.h>

#include "game/race_internal.h"

int main(void) {
    assert(BgmCdTrack(0) == 3);
    assert(BgmCdTrack(8) == 11);
    assert(BgmCdTrack(9) == 17);
    assert(BgmCdTrack(10) == 13);
    assert(WrapBgmTrackIndex(-1, 10) == 9);
    assert(WrapBgmTrackIndex(0, 10) == 0);
    assert(WrapBgmTrackIndex(10, 10) == 0);
    assert(WrapBgmTrackIndex(21, 10) == 1);
    assert(WrapBgmTrackIndex(3, 0) == 0);
    return 0;
}
