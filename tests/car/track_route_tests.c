#include "game/track.h"
#include <stdio.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

typedef struct Fixture { s32 count; GameTrackPoint points[2]; GameTrackArcCenter arcs[1]; } Fixture;
int main(void) {
    Fixture first = {.count = 2};
    first.points[0].segmentLength = 100;
    first.points[1].segmentLength = 200;
    first.points[0].arcRef = TRACK_CURVE_PRIMARY;
    Fixture second = first;
    second.points[0].segmentLength = 500;
    TrackRoute a, b;
    CHECK(ReadTrackRoute((const TrackPointTable *)&first, sizeof(first), &a));
    CHECK(a.points == first.points && a.arcs == first.arcs && a.count == 2 && a.length == 300);
    CHECK(ReadTrackRoute((const TrackPointTable *)&second, sizeof(second), &b));
    CHECK(b.points == second.points && b.length == 700 && a.length == 300);
    for (size_t size = 0; size < sizeof(first); size++) {
        CHECK(!ReadTrackRoute((const TrackPointTable *)&first, size, &b));
        CHECK(b.points == NULL && b.arcs == NULL && b.count == 0 && b.length == 0);
    }
    first.points[0].arcRef = (1 << 4) | TRACK_CURVE_PRIMARY;
    CHECK(!ReadTrackRoute((const TrackPointTable *)&first, sizeof(first), &b));
    first.points[0].arcRef = 0;
    first.points[1].segmentLength = 0;
    CHECK(!ReadTrackRoute((const TrackPointTable *)&first, sizeof(first), &b));
    CHECK(!ReadTrackRoute(NULL, 0, &b));
    CHECK(!ReadTrackRoute((const TrackPointTable *)&second, sizeof(second), NULL));
    CHECK(a.points == first.points && a.length == 300);
    return 0;
}
