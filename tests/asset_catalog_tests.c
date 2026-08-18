#include "game/asset_catalog.h"

#include <assert.h>

int main(void) {
    assert(!AssetIdIsValid(ASSET_ID_INVALID));
    assert(AssetIdIsValid(ASSET_ID_BOOT));
    assert(AssetIdIsValid(ASSET_ID_COUNT - 1));
    assert(!AssetIdIsValid(ASSET_ID_COUNT));

    assert(AssetRoundScreenId(0, 0) == 0x4A);
    assert(AssetRoundScreenId(1, 5) == 0x55);
    assert(AssetRoundScreenId(-1, 0) == ASSET_ID_INVALID);
    assert(AssetRoundScreenId(0, ASSET_CLASS_COUNT) == ASSET_ID_INVALID);

    assert(AssetTrackPackId(0, 0, 0) == 0x57);
    assert(AssetTrackPackId(0, 0, 1) == 0x58);
    assert(AssetTrackPackId(0, 4, 0) == 0x5F);
    assert(AssetTrackPackId(5, 3, 1) == 0x86);
    assert(AssetTrackPackId(-1, 0, 0) == ASSET_ID_INVALID);
    assert(AssetTrackPackId(0, ASSET_PHYSICAL_COURSE_COUNT, 0) == ASSET_ID_INVALID);
    assert(AssetTrackPackId(5, 4, 0) == ASSET_ID_INVALID);
    assert(AssetTrackPackId(0, 0, 2) == ASSET_ID_INVALID);
    return 0;
}
