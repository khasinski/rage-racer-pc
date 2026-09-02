#include "game/asset.h"
#include "game/car.h"
#include "game/race.h"
#include "game/random.h"
#include "game/render.h"
#include "game/state.h"

enum {
    ATTRACT_CAMERA_CAR_COUNT = 4
};

static s32 CycleCameraCar(s32 mask, s32 current, s32 carCount) {
    s32 candidate;
    s32 currentPage;

    if ((u32)current >= (u32)carCount) {
        current = 0;
    }
    if ((mask & g_SceneTimer) != 0) {
        return current;
    }
    if (g_TrackTextureCursorRow != 0 &&
        g_TrackTextureCursorRow != TRACK_TEXTURE_PAGE_ROW_COUNT) {
        return current;
    }

    candidate = (Random15() & 0x7FFF) % carCount;
    currentPage = TrackTexturePageForSection(g_Cars[current].trackSection);
    if (currentPage ==
        TrackTexturePageForSection(g_Cars[candidate].trackSection)) {
        return candidate;
    }
    return current;
}

s32 CycleBgmSelectCameraCar(s32 mask, s32 current) {
    return CycleCameraCar(mask, current, RACE_CAR_SLOT_COUNT);
}

s32 CycleAttractCameraCar(s32 mask, s32 current) {
    return CycleCameraCar(mask, current, ATTRACT_CAMERA_CAR_COUNT);
}
