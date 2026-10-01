/* Network diagnostics for players (web_netstats.c), read by scripts/net-check.mjs. */
#ifndef WEB_NETSTATS_H
#define WEB_NETSTATS_H
#include "game/race_sim.h"

/* One applied server frame: where the driving cars were before it
 * (`before`, for the seats `driving`), how many ticks were replayed. */
void WebNetStatsRecord(const RaceSim *sim, int localSeat, const s32 before[DRIVER_SEAT_LIMIT][3],
                       const int driving[DRIVER_SEAT_LIMIT], u32 replayed);
#endif
