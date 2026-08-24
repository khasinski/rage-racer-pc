#include "render_world.h"

#include <string.h>

void RageRenderWorldInit(RageRenderWorld *world,
                         RageRenderMeshInstance *instances,
                         uint32_t capacity) {
    memset(world, 0, sizeof(*world));
    world->instances = instances;
    world->instanceCapacity = capacity;
}

void RageRenderWorldBeginFrame(RageRenderWorld *world, uint64_t frame) {
    if (world->hasCamera) world->previousCamera = world->camera;
    world->frame = frame;
    world->instanceCount = 0;
    world->overflowCount = 0;
}

void RageRenderWorldSetCamera(RageRenderWorld *world,
                              const RageRenderCamera *camera) {
    world->camera = *camera;
    if (!world->hasCamera) world->previousCamera = *camera;
    world->hasCamera = 1;
}

int RageRenderWorldSubmitMesh(RageRenderWorld *world,
                              const RageRenderMeshInstance *instance) {
    if (world->instanceCount == world->instanceCapacity) {
        world->overflowCount++;
        return 0;
    }
    world->instances[world->instanceCount++] = *instance;
    return 1;
}

void RageRenderTerrainCellTransform(uint32_t grid_x, uint32_t grid_z,
                                    RageRenderTransform *transform) {
    memset(transform, 0, sizeof(*transform));
    /* Original cells are in an inverted 32x32 grid and use 8192 mesh units. */
    transform->position.x = (float)(grid_x * 8192u + 4096u);
    transform->position.z = (float)((31u - grid_z) * 8192u + 4096u);
    transform->scale.x = 1.0f;
    transform->scale.y = 1.0f;
    transform->scale.z = 1.0f;
}

