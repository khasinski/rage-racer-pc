#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/track_internal.h"

enum {
    RIVAL_CUE_ZONE_START = 0x7001,
    RIVAL_CUE_FINISH_MARGIN = 0x3000,
    WRONG_WAY_CUE_MUTE_FRAMES = 10,
};

void UpdateRivalCueGate(void) {
    const s32 progress = g_PlayerCar.trackProgress;
    const s32 inCueZone =
        progress >= RIVAL_CUE_ZONE_START &&
        progress < g_TrackLength - RIVAL_CUE_FINISH_MARGIN;

    if (inCueZone) {
        g_RivalCueEnabled = 1;
    } else if (g_RivalCueEnabled == 1) {
        /* State 2 is deliberately sticky outside the zone: lap cue playback
         * uses it to keep rival speech enabled after its delay expires. */
        g_RivalCueEnabled = 0;
    }

    if (g_WrongWayTimer >= WRONG_WAY_CUE_MUTE_FRAMES) {
        g_RivalCueEnabled = 0;
    }
}
