#include "ray_world.h"

#include <stddef.h>
#include <stdlib.h>

int RaySceneBuildWorld(RayScene *scene, const RenderWorld *world,
                       RenderPass pass, RayWorldMeshLookup lookup,
                       void *context) {
    RayInstance *instances;
    size_t count = 0;
    int built;

    if (scene == NULL || world == NULL || lookup == NULL ||
        (world->instanceCount != 0 && world->instances == NULL)) return 0;
    if (world->instanceCount == 0) return RaySceneBuild(scene, NULL, 0);
    if (sizeof(*instances) > SIZE_MAX / world->instanceCount) return 0;
    instances = malloc((size_t)world->instanceCount * sizeof(*instances));
    if (instances == NULL) return 0;
    for (uint32_t index = 0; index < world->instanceCount; ++index) {
        const RenderMeshInstance *source = &world->instances[index];
        const RayMesh *mesh;
        uint32_t flags = 0;

        if (source->pass != pass) continue;
        if (source->flags & RAGE_RENDER_INSTANCE_LAMPS_ONLY) continue;
        mesh = lookup(context, source);
        if (mesh == NULL) continue;
        if (source->flags & RAGE_RENDER_INSTANCE_CULL_BACKFACES)
            flags |= RAY_INSTANCE_CULL_BACKFACES;
        if (source->flags & RAGE_RENDER_INSTANCE_RAY_NO_SHADOW)
            flags |= RAY_INSTANCE_NO_SHADOW;
        if (!RayInstancePrepare(&instances[count], mesh, &source->transform,
                                source->entity, flags)) {
            free(instances);
            return 0;
        }
        ++count;
    }
    built = RaySceneBuild(scene, instances, count);
    free(instances);
    return built;
}
