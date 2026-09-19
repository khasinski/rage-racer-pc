#ifndef RAGE_RAY_DRAWS_H
#define RAGE_RAY_DRAWS_H

#include "ray_bvh.h"
#include "render/render_native_vertex.h"

typedef int (*RayDrawInclude)(void *context,
                              const RageNativeDrawSpan *span);

/* Flatten already-expanded world draw ranges into one tracing BVH. The caller
 * decides which spans are opaque shadow casters. */
int RayMeshBuildDraws(RayMesh *mesh, const RageNativeGpuVertex *vertices,
                      size_t vertexCount, const RageNativeDrawSpan *spans,
                      size_t spanCount, RayDrawInclude include, void *context);

#endif
