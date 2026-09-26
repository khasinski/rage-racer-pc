#ifndef GAME_GEOMETRY_H
#define GAME_GEOMETRY_H

#include "common.h"

/* Twice the signed triangle area. Coordinates are packed signed 16-bit x/y
 * pairs; the result wraps to signed 32-bit, preserving retail clipping. */
s32 TriangleArea(u32 first, u32 second, u32 third);

#endif
