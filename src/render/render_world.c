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
    if (world->hasMirrorCamera) {
        world->previousMirrorCamera = world->mirrorCamera;
        world->previousMirrorPanelY = world->mirrorPanelY;
    }
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

void RageRenderWorldSetMirrorCamera(RageRenderWorld *world,
                                    const RageRenderCamera *camera,
                                    int active, float panelY) {
    world->mirrorCamera = *camera;
    world->mirrorPanelY = panelY;
    world->mirrorActive = active != 0;
    if (!world->hasMirrorCamera) {
        world->previousMirrorCamera = *camera;
        world->previousMirrorPanelY = panelY;
    }
    world->hasMirrorCamera = 1;
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

void RageRenderConvertPsxMatrix(const float source[3][3], float out[3][3]) {
    static const float sign[3] = {1.0f, -1.0f, -1.0f};
    int row, column;
    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            out[row][column] = source[row][column] * sign[row] * sign[column];
        }
    }
}
