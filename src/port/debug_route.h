#ifndef RAGE_DEBUG_ROUTE_H
#define RAGE_DEBUG_ROUTE_H
#include <stdint.h>

typedef struct DebugRoutePoint { int x, z, angle, length; } DebugRoutePoint;
typedef DebugRoutePoint (*DebugRouteRead)(void *context, int index);
typedef struct DebugRoute {
    int point, offset, remainder, segments, laps;
    uint64_t distance;
} DebugRoute;
typedef struct DebugRouteSample { int x, z, heading; } DebugRouteSample;
/* Distances use track units, speed uses units/second. No game/GPU state. */
int DebugRouteAdvance(DebugRoute *route, int count, int direction, int speed,
                      int hz, DebugRouteRead read, void *context,
                      DebugRouteSample *sample);
#endif
