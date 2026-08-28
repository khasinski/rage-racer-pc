#ifndef RAGE_RENDER_WORLD_SNAPSHOT_H
#define RAGE_RENDER_WORLD_SNAPSHOT_H

#include "render_world.h"

typedef struct RageRenderWorldSnapshot {
    RageRenderWorld world;
    RageRenderMeshInstance *instances;
} RageRenderWorldSnapshot;

/* A versioned, little-endian copy of the complete renderer-neutral input.
 * The file deliberately contains no pointers and no PS1 ordering-table data. */
int RenderWorldSnapshotWrite(const char *path,
                                 const RageRenderWorld *world);
int RenderWorldSnapshotRead(const char *path,
                                RageRenderWorldSnapshot *snapshot);
void RenderWorldSnapshotRelease(RageRenderWorldSnapshot *snapshot);

#endif
