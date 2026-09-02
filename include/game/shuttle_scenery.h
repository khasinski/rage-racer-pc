#ifndef GAME_SHUTTLE_SCENERY_H
#define GAME_SHUTTLE_SCENERY_H

#include "common.h"
#include "game/vector.h"

/* Runtime state of a prop travelling back and forth between two endpoints. */
typedef struct GameShuttleScenery {
    s32 dwellCounter;  /* frames waited at the endpoint */
    s32 reserved04;
    s32 travelStep;    /* progress along the current leg */
    s16 startEndpoint; /* endpoint from which the current leg started */
    s16 pathIndex;
    Vec4 position;     /* interpolated world position */
    s32 angleX;        /* seeded from the path, unused by the drawer */
    s32 angleY;
    s32 angleZ;
    u8 pad2C[8];
} GameShuttleScenery;

_Static_assert(sizeof(GameShuttleScenery) == 0x34,
               "GameShuttleScenery must match the retail layout");

#endif
