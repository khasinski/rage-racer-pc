#ifndef RAGE_RENDER_WORLD_SNAPSHOT_H
#define RAGE_RENDER_WORLD_SNAPSHOT_H

#include "render_world.h"

typedef struct RageRenderWorldSnapshot {
    RageRenderWorld world;
    RageRenderMeshInstance *instances;
} RageRenderWorldSnapshot;

/* A versioned, little-endian copy of the complete renderer-neutral input.
 * The file deliberately contains no pointers and no PS1 ordering-table data. */
int RageRenderWorldSnapshotWrite(const char *path,
                                 const RageRenderWorld *world);
int RageRenderWorldSnapshotRead(const char *path,
                                RageRenderWorldSnapshot *snapshot);
void RageRenderWorldSnapshotRelease(RageRenderWorldSnapshot *snapshot);

#endif
