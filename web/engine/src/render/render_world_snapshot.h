#ifndef RAGE_RENDER_WORLD_SNAPSHOT_H
#define RAGE_RENDER_WORLD_SNAPSHOT_H

#include "render_world.h"

typedef struct RenderWorldSnapshot {
    RenderWorld world;
    RenderMeshInstance *instances;
} RenderWorldSnapshot;

/* A versioned, little-endian copy of the complete renderer-neutral input.
 * The file deliberately contains no pointers and no PS1 ordering-table data.
 * Writing reserves path.tmp exclusively; a pre-existing reservation is left
 * untouched and causes failure. Successful staging is renamed to the target. */
int RenderWorldSnapshotWrite(const char *path,
                                 const RenderWorld *world);
/* Destination must be zero-initialized or own a previous snapshot. Successful
 * reads replace/release it; failed reads preserve its data and ownership. */
int RenderWorldSnapshotRead(const char *path,
                                RenderWorldSnapshot *snapshot);
void RenderWorldSnapshotRelease(RenderWorldSnapshot *snapshot);
/* Deep-copy renderer input into a zero-initialized or owned snapshot. Source
 * may alias the destination world. Failure preserves the previous snapshot.
 * Reuses owned instance capacity when sufficient; copies still own all values.
 * Mesh/material resources referenced by IDs are not copied here. */
int RenderWorldSnapshotCopy(RenderWorldSnapshot *snapshot, const RenderWorld *world);

#endif
