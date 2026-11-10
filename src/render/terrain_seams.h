#ifndef RAGE_RENDER_TERRAIN_SEAMS_H
#define RAGE_RENDER_TERRAIN_SEAMS_H
#include <math.h>
#include "render_mesh_build.h"

typedef struct RenderTerrainSeams RenderTerrainSeams;
static inline float RenderSnapTerrainCellBoundary(float value) {
    float edge=roundf(value/2048.0f)*2048.0f;
    return fabsf(value-edge)<=0.5f?edge:value;
}
/* Immutable source terrain only. The cache follows the native mesh-cache
 * lifetime, and retains correspondence when a neighbour leaves the view. */
void RenderTerrainSeamsPrepare(RenderTerrainSeams **cache,
    const RenderWorld *world,int pass,RageRenderMeshLookup lookup,void *context);
Vec3 RenderTerrainSeamsPosition(RenderTerrainSeams *cache,const RenderMeshInstance *instance,Vec3 point);
void RenderTerrainSeamsRelease(RenderTerrainSeams *cache);
#endif
