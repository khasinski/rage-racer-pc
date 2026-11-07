#ifndef GAME_HULL_ROTATION_H
#define GAME_HULL_ROTATION_H
#include "game/car.h"

typedef struct HullAxes { s16 xx, xz, zx, zz; } HullAxes;
HullAxes BuildHullAxes(s16 pitch, s16 yaw, s16 roll);
LVec RotateHullPoint(const HullAxes *axes, const CarHullPoint *point);
#endif
