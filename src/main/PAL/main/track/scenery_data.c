#include "game/shuttle_scenery.h"
#include "game/spinners.h"

ShuttlePath g_ShuttlePathPoints[3] = {
    {{{20908, 3188, 37947, 0}, {27399, 250, 34015, 0}}},
    {{{6934, 1271, 31155, 0}, {9723, 1547, 29190, 0}}},
    {{{9762, 1566, 28960, 0}, {6887, 1275, 30981, 0}}},
};
SVec g_ShuttlePathAngles[3] = {
    {0, 3736, -240, 0},
    {0, 10234, 0, 0},
    {0, 10234, 0, 0}
};
s16 g_ShuttlePathTravelMax[4] = {
    628, 512, 512, 0
};
s16 g_ShuttlePathDwellMax[62] = {
    300, 128, 128, 0, -17324, 0, 5903, 0, 12361, 0, 0, 0, 200, 180, -19127,
    0, 5903, 0, 12309, 0, 0, 0, 400, 180, -18381, 0, 5903, 0, 12444, 0, 0,
    0, 600, 180, -17324, 0, 5903, 0, 12361, 0, 0, 0, -1, 180, 200, 0, 0, 0,
    100, 100, 180, 2048, 200, 0, 200, 100, 200, 0, 0, 0, -1, 100
};
int RetailShuttle(s32 path, ShuttleConfig *config) {
    if (!config || (u32)path >= SHUTTLE_PATH_COUNT) return 0;
    *config = (ShuttleConfig){g_ShuttlePathPoints[path], g_ShuttlePathAngles[path],
                              g_ShuttlePathTravelMax[path], g_ShuttlePathDwellMax[path]};
    return 1;
}

SpinningSceneryPlacement g_SpinningSceneryPlacements[4] = {
        {{17805, 5646, 44714}, 590},
        {{30065, 3143, 40558}, 1995},
        {{30171, 3054, 38836}, 2007},
        {{30888, 2954, 37357}, 2005},
    };
int RetailSpinner(s32 index, SpinningSceneryPlacement *placement) {
    if (!placement || (u32)index >= 4) return 0;
    *placement = g_SpinningSceneryPlacements[index];
    return 1;
}
