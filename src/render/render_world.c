#include "render_world.h"

#include <math.h>
#include <string.h>

static float RenderWrappedAngleDelta(float from, float to) {
    float delta = to - from;
    while (delta > 180.0f) delta -= 360.0f;
    while (delta < -180.0f) delta += 360.0f;
    return fabsf(delta);
}

static int RenderCameraIsCut(const RageRenderCamera *previous,
                                 const RageRenderCamera *current) {
    float dx = current->transform.position.x - previous->transform.position.x;
    float dy = current->transform.position.y - previous->transform.position.y;
    float dz = current->transform.position.z - previous->transform.position.z;
    if (dx * dx + dy * dy + dz * dz > 1024.0f * 1024.0f) return 1;
    if (previous->transform.hasOrientation &&
        current->transform.hasOrientation) {
        float dot = previous->transform.orientation.x *
                        current->transform.orientation.x +
                    previous->transform.orientation.y *
                        current->transform.orientation.y +
                    previous->transform.orientation.z *
                        current->transform.orientation.z +
                    previous->transform.orientation.w *
                        current->transform.orientation.w;
        /* Quaternion dot is cos(half the angular distance). A change above
         * 45 degrees in one logic tick is a shot cut, not camera motion. */
        if (fabsf(dot) < 0.9238795f) return 1;
    } else if (RenderWrappedAngleDelta(
                   previous->transform.rotation.x,
                   current->transform.rotation.x) > 45.0f ||
               RenderWrappedAngleDelta(
                   previous->transform.rotation.y,
                   current->transform.rotation.y) > 45.0f ||
               RenderWrappedAngleDelta(
                   previous->transform.rotation.z,
                   current->transform.rotation.z) > 45.0f) {
        return 1;
    }
    return fabsf(current->verticalFovDegrees -
                  previous->verticalFovDegrees) > 10.0f;
}

void RenderDirectionalLightDefault(RageRenderDirectionalLight *light) {
    if (light == NULL) return;
    light->direction = (RageRenderVec3){-0.1f, 1.0f, 0.12f};
    light->ambientColor = (RageRenderVec3){0.35f, 0.35f, 0.35f};
    light->diffuseColor = (RageRenderVec3){0.65f, 0.65f, 0.65f};
}

void RenderWorldInit(RageRenderWorld *world,
                         RageRenderMeshInstance *instances,
                         uint32_t capacity) {
    memset(world, 0, sizeof(*world));
    world->instances = instances;
    world->instanceCapacity = capacity;
    RenderDirectionalLightDefault(&world->light);
}

void RenderWorldBeginFrame(RageRenderWorld *world, uint64_t frame) {
    if (world->hasCamera) world->previousCamera = world->camera;
    if (world->hasMirrorCamera) {
        world->previousMirrorCamera = world->mirrorCamera;
        world->previousMirrorPanelY = world->mirrorPanelY;
    }
    world->frame = frame;
    world->instanceCount = 0;
    world->overflowCount = 0;
}

void RenderWorldSetDirectionalLight(
    RageRenderWorld *world, const RageRenderDirectionalLight *light) {
    if (world == NULL || light == NULL) return;
    world->light = *light;
}

void RenderWorldSetCamera(RageRenderWorld *world,
                              const RageRenderCamera *camera) {
    if (world->hasCamera &&
        RenderCameraIsCut(&world->previousCamera, camera))
        world->previousCamera = *camera;
    world->camera = *camera;
    if (!world->hasCamera) world->previousCamera = *camera;
    world->hasCamera = 1;
}

void RenderWorldSetMirrorCamera(RageRenderWorld *world,
                                    const RageRenderCamera *camera,
                                    int active, float panelY) {
    if (world->hasMirrorCamera &&
        RenderCameraIsCut(&world->previousMirrorCamera, camera)) {
        world->previousMirrorCamera = *camera;
        world->previousMirrorPanelY = panelY;
    }
    world->mirrorCamera = *camera;
    world->mirrorPanelY = panelY;
    world->mirrorActive = active != 0;
    if (!world->hasMirrorCamera) {
        world->previousMirrorCamera = *camera;
        world->previousMirrorPanelY = panelY;
    }
    world->hasMirrorCamera = 1;
}

int RenderWorldSubmitMesh(RageRenderWorld *world,
                          const RageRenderMeshInstance *instance) {
    if (world == NULL || instance == NULL) return 0;
    if (world->instances == NULL ||
        world->instanceCount >= world->instanceCapacity) {
        world->overflowCount++;
        return 0;
    }
    world->instances[world->instanceCount++] = *instance;
    return 1;
}

void RenderWorldDiscardPass(RageRenderWorld *world, RageRenderPass pass) {
    uint32_t source, destination = 0;
    if (world == NULL) return;
    for (source = 0; source < world->instanceCount; source++) {
        if (world->instances[source].pass == pass) continue;
        if (destination != source)
            world->instances[destination] = world->instances[source];
        destination++;
    }
    world->instanceCount = destination;
}

void RenderTerrainCellTransform(uint32_t grid_x, uint32_t grid_z,
                                    RageRenderTransform *transform) {
    memset(transform, 0, sizeof(*transform));
    /* Original cells are in an inverted 32x32 grid and use 8192 mesh units. */
    transform->position.x = (float)(grid_x * 8192u + 4096u);
    transform->position.z = (float)((31u - grid_z) * 8192u + 4096u);
    transform->scale.x = 1.0f;
    transform->scale.y = 1.0f;
    transform->scale.z = 1.0f;
}

void RenderConvertPsxMatrix(const float source[3][3], float out[3][3]) {
    static const float sign[3] = {1.0f, -1.0f, -1.0f};
    int row, column;
    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            out[row][column] = source[row][column] * sign[row] * sign[column];
        }
    }
}
