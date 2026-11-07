/* Network diagnostics for players: how far each server frame moved the
 * predicted cars (world units), own car and the others, and how many ticks
 * were replayed. Read and reset by the network checks (scripts/net-check.mjs). */
#include <emscripten/emscripten.h>
#include <math.h>
#include <string.h>
#include "web_netstats.h"

static struct NetStats {
    double frames, ownSum, ownMax, otherSum, otherMax, otherCount, replaySum;
} s_netStats;
static float s_netStatsOut[6];

/* Frames, own mean and max, others' mean and max, mean ticks replayed. */
EMSCRIPTEN_KEEPALIVE const float *rw_net_stats(void) {
    const double frames = s_netStats.frames > 0 ? s_netStats.frames : 1;
    const double others = s_netStats.otherCount > 0 ? s_netStats.otherCount : 1;
    s_netStatsOut[0] = (float)s_netStats.frames;
    s_netStatsOut[1] = (float)(s_netStats.ownSum / frames);
    s_netStatsOut[2] = (float)s_netStats.ownMax;
    s_netStatsOut[3] = (float)(s_netStats.otherSum / others);
    s_netStatsOut[4] = (float)s_netStats.otherMax;
    s_netStatsOut[5] = (float)(s_netStats.replaySum / frames);
    return s_netStatsOut;
}
EMSCRIPTEN_KEEPALIVE void rw_net_stats_reset(void) { memset(&s_netStats, 0, sizeof(s_netStats)); }

void WebNetStatsRecord(const RaceSim *sim, int localSeat, const s32 before[DRIVER_SEAT_LIMIT][3],
                       const int driving[DRIVER_SEAT_LIMIT], u32 replayed) {
    s_netStats.frames += 1;
    s_netStats.replaySum += replayed;
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
        const PlayerCarRuntime *car = &sim->drivers[seat].car;
        if (!driving[seat] || sim->drivers[seat].status != SIM_DRIVING) continue;
        const double dx = car->x - before[seat][0], dy = car->y - before[seat][1], dz = car->z - before[seat][2];
        const double moved = sqrt(dx * dx + dy * dy + dz * dz);
        if (seat == localSeat) {
            s_netStats.ownSum += moved;
            if (moved > s_netStats.ownMax) s_netStats.ownMax = moved;
        } else {
            s_netStats.otherSum += moved;
            s_netStats.otherCount += 1;
            if (moved > s_netStats.otherMax) s_netStats.otherMax = moved;
        }
    }
}
