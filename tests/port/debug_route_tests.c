#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include "debug_route.h"

static DebugRoutePoint points[] = {
    {0, 0, 4090, 100}, {100, 0, 6, 100},
    {100, 100, 1024, 100}, {0, 100, 2048, 100}
};
static DebugRoutePoint Read(void *ctx, int index) {
    return ((DebugRoutePoint *)ctx)[index];
}
static DebugRoute Run(int hz, int direction) {
    DebugRoute r = {0};
    DebugRouteSample out;
    for (int i = 0; i < hz * 12; i++)
        assert(DebugRouteAdvance(&r, 4, direction, 137, hz, Read, points, &out));
    assert(r.distance == 1644 && r.laps == 4 && r.offset == 44);
    return r;
}
int main(void) {
    DebugRoute a = Run(50, 1), b = Run(60, 1);
    DebugRoute reverse = Run(50, -1), r = {0};
    DebugRoute palRace = Run(25, 1), ntscRace = Run(30, 1);
    DebugRouteSample out;
    assert(a.point == b.point && a.offset == b.offset);
    assert(palRace.point == ntscRace.point && palRace.offset == ntscRace.offset);
    assert(reverse.laps == a.laps);
    assert(DebugRouteAdvance(&r, 4, 1, 50, 1, Read, points, &out));
    assert(out.x == 50 && out.z == 0 && out.heading == 1024);
    r = (DebugRoute){0};
    assert(DebugRouteAdvance(&r, 4, -1, 50, 1, Read, points, &out));
    assert(out.x == 0 && out.z == 50 && r.point == 0);
    r = (DebugRoute){0};
    assert(DebugRouteAdvance(&r, 4, 1, 1200, 1, Read, points, &out));
    assert(r.laps == 3 && r.point == 0 && r.offset == 0);
    assert(!DebugRouteAdvance(&r, 1, 1, 50, 50, Read, points, &out));
    assert(!DebugRouteAdvance(&r, 4, 0, 50, 50, Read, points, &out));
    assert(!DebugRouteAdvance(&r, 4, 1, 50, 0, Read, points, &out));
    assert(!DebugRouteAdvance(&r, 4, 1, INT_MAX, 50, Read, points, &out));
    points[0].length = 0;
    assert(!DebugRouteAdvance(&r, 4, 1, 50, 50, Read, points, &out));
    puts("debug route: loops, reverse, interpolation, PAL/NTSC pacing and invalid input passed");
    return 0;
}
