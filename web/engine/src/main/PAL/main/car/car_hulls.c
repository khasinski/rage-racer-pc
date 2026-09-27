#include "game/car.h"

/* Shared immutable retail geometry: car contacts and road contacts differ. */
const CarHullPoint g_PlayerHullPoints[6] = {
    {-32, 64}, {32, 64}, {-24, -72}, {24, -72}, {-32, 16}, {32, 16}
};
const CarHullPoint g_OpponentHullCorners[4] = {
    {-26, 96}, {26, 96}, {-26, -16}, {26, -16}
};
const CarHullPoint g_CarCornerOffsets[4] = {
    {-15, 20}, {15, 20}, {-8, -10}, {8, -10}
};
const CarHullPoint g_CarCollisionCorners[4] = {
    {-96, 512}, {96, 512}, {-96, -128}, {96, -128}
};
