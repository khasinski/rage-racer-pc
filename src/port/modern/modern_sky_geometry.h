#ifndef MODERN_SKY_GEOMETRY_H
#define MODERN_SKY_GEOMETRY_H

#include <math.h>

/* Captured UVs belong to the source tile, not to the interpolated tile
 * counter. Keep that identity attached to its continuous screen position,
 * including the 31 -> 0 camera-yaw wrap. */
static inline float ModernSkySourceColumn(float source, float target) {
    float delta = source - target;
    while (delta > 16.0f) delta -= 32.0f;
    while (delta < -16.0f) delta += 32.0f;
    return delta;
}

/* Untextured bands have constant colour along each horizontal edge. Extend
 * those edges in their rolled direction, not along screen X, so widening
 * does not change the horizon or its gradient. Covers the viewport diagonal
 * plus the authored off-centre origin. Never apply this to textured clouds. */
static inline void ModernSkyExtendBand(float x[4], float y[4], float width) {
    float dx = x[1] - x[0], dy = y[1] - y[0];
    float length = hypotf(dx, dy);
    float extra = hypotf(width, 240.0f) + 512.0f;
    if (length < 0.001f) return;
    dx *= extra / length;
    dy *= extra / length;
    for (int i = 0; i < 4; ++i) {
        float sign = (i & 1) ? 1.0f : -1.0f;
        x[i] += sign * dx;
        y[i] += sign * dy;
    }
}

#endif
