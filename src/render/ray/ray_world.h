#ifndef RAGE_RAY_WORLD_H
#define RAGE_RAY_WORLD_H

#include "ray_scene.h"

typedef const RayMesh *(*RayWorldMeshLookup)(
    void *context, const RenderMeshInstance *instance);

/* Convert one semantic render pass into traceable instances. A lookup returning
 * NULL deliberately excludes that instance (for example transparent geometry
 * in the first opaque-only implementation). */
int RaySceneBuildWorld(RayScene *scene, const RenderWorld *world,
                       RenderPass pass, RayWorldMeshLookup lookup,
                       void *context);

#endif
