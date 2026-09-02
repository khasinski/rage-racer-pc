#include <assert.h>

#include "game/race_internal.h"

int main(void) {
    assert(BgmCdTrack(0) == 3);
    assert(BgmCdTrack(8) == 11);
    assert(BgmCdTrack(9) == 17);
    assert(BgmCdTrack(10) == 13);
    return 0;
}
