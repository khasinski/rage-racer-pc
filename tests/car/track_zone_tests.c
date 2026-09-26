#include "game/track.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    TrackEventData first = {0}, second = {0};
    first.zones[0] = (TrackZone){100, 1000, 1, 7};
    first.zones[1].start = -1;
    second.zones[0] = (TrackZone){300, 1400, 0, 9};
    second.zones[1].start = -1;
    TrackEventData savedFirst = first, savedSecond = second;
    for (s32 position = -100; position < 2200; ++position) {
        TrackZoneEffect a = ReadTrackZoneEffect(&first, position, 2000, 0);
        TrackZoneEffect b = ReadTrackZoneEffect(&second, 2000 - position, 2000, 1);
        CHECK(a.reverb == (position > 100 && position < 1000 ? 7 : 0));
        CHECK(b.reverb == (position > 300 && position < 1400 ? 9 : 0));
        if (position > 300 && position < 1400) CHECK(b.dark == 3);
        TrackZoneEffect again = ReadTrackZoneEffect(&first, position, 2000, 0);
        CHECK(memcmp(&a, &again, sizeof(a)) == 0);
    }
    CHECK(memcmp(&first, &savedFirst, sizeof(first)) == 0);
    CHECK(memcmp(&second, &savedSecond, sizeof(second)) == 0);
    CHECK(ReadTrackZoneEffect(&first, 101, 2000, 0).blend == 1);
    CHECK(ReadTrackZoneEffect(&first, 500, 2000, 0).blend == 256);
    CHECK(ReadTrackZoneEffect(&first, 999, 2000, 0).blend == 1);
    first.zones[0].code = 2;
    CHECK(ReadTrackZoneEffect(&first, 500, 2000, 0).blend == 0);
    first.zones[0].code = -3;
    CHECK(ReadTrackZoneEffect(&first, 101, 2000, 0).blend == 1);
    CHECK(ReadTrackZoneEffect(&first, 999, 2000, 0).blend == 256);
    first.zones[0].code = INT16_MIN;
    CHECK(ReadTrackZoneEffect(&first, 101, 2000, 0).code == INT16_MAX);
    first.zones[0] = (TrackZone){INT_MIN, INT_MAX, 1, 7};
    CHECK(ReadTrackZoneEffect(&first, 0, 2000, 0).blend == 256);
    CHECK(ReadTrackZoneEffect(NULL, 500, 2000, 0).blend == 0);
    return 0;
}
