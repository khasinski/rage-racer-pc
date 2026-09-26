#ifndef GAME_SHUTTLE_SCENERY_H
#define GAME_SHUTTLE_SCENERY_H

#include "common.h"
#include "game/vector.h"

typedef struct ShuttlePath {
    Vec4 endpoint[2];
} ShuttlePath;

enum {
    SHUTTLE_INSTANCE_COUNT = 2,
    SHUTTLE_PATH_COUNT = 3,
    SHUTTLE_ENDPOINT_COUNT = 2,
};

typedef struct ShuttleConfig {
    ShuttlePath path;
    SVec angles;
    s32 travel, dwell;
} ShuttleConfig;
/* Copies authored configuration; runtime playback does not borrow it. */
int RetailShuttle(s32 path, ShuttleConfig *config);

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

/* Caller-owned playback. Invalid input leaves state unchanged. The step
 * preserves retail wrapped integer interpolation and endpoint dwell timing. */
int InitShuttle(GameShuttleScenery *state, const ShuttlePath *path, const SVec *angles,
                s32 pathIndex, s32 dwell);
int StepShuttle(GameShuttleScenery *state, const ShuttlePath *path, s32 duration, s32 dwell);
#endif
