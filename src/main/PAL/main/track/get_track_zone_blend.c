#include "game/race.h"
#include "game/track.h"

TrackZoneEffect GetTrackZoneEffect(s32 position) {
    return ReadTrackZoneEffect(g_TrackEventData, position, g_TrackLength, g_RaceSeries);
}
