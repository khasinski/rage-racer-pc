#include "render_world.h"

#include <math.h>
#include <string.h>

static float RenderWrappedAngleDelta(float from, float to) {
    float delta = to - from;

    if (!isfinite(delta)) return INFINITY;
    delta = fmodf(delta, 360.0f);
    if (delta > 180.0f) delta -= 360.0f;
    if (delta < -180.0f) delta += 360.0f;
    return fabsf(delta);
}

static int RenderCameraIsCut(const RenderCamera *previous,
                                 const RenderCamera *current) {
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

void RenderDirectionalLightDefault(RenderDirectionalLight *light) {
    if (light == NULL) return;
    /* Keep vehicle shadows visible beside their casters.  A nearly vertical
     * light hid the whole footprint beneath each car from the chase camera. */
    light->direction = (Vec3){-0.34f, 1.0f, 0.42f};
    light->ambientColor = (Vec3){0.35f, 0.35f, 0.35f};
    light->diffuseColor = (Vec3){0.65f, 0.65f, 0.65f};
}

static float RenderLightClamp(float value, float minimum, float maximum) {
    return value < minimum ? minimum : value > maximum ? maximum : value;
}

static float RenderLightLuminance(Vec3 color) {
    return color.x * 0.2126f + color.y * 0.7152f + color.z * 0.0722f;
}

void RenderDirectionalLightFromSky(const RenderCamera *camera,
                                   RenderDirectionalLight *light) {
    float sky, horizon, daylight, elevation, horizontal, tintMaximum;
    Vec3 tint;

    if (camera == NULL || light == NULL) return;
    sky = RenderLightLuminance(camera->skyTopColor);
    horizon = RenderLightLuminance(camera->skyHorizonColor);
    daylight = RenderLightClamp(fmaxf(sky, horizon) * 1.35f, 0.08f, 1.0f);

    /* The retail environment script animates these sky bands through the
     * course.  Bright daylight places the sun high; dark and sunset palettes
     * produce a lower, longer shadow without tying it to the camera. */
    elevation = 0.28f + daylight * 0.58f;
    horizontal = sqrtf(fmaxf(0.0f, 1.0f - elevation * elevation));
    light->direction = (Vec3){-horizontal * 0.63f, elevation,
                              horizontal * 0.7766f};

    /* The horizon carries the sun's warm/cool cast.  Normalize its hue before
     * applying intensity so a dark evening sky does not turn the light black. */
    tintMaximum = fmaxf(camera->skyHorizonColor.x,
                        fmaxf(camera->skyHorizonColor.y,
                              camera->skyHorizonColor.z));
    if (tintMaximum > 0.001f) {
        tint.x = RenderLightClamp(camera->skyHorizonColor.x / tintMaximum,
                                  0.38f, 1.0f);
        tint.y = RenderLightClamp(camera->skyHorizonColor.y / tintMaximum,
                                  0.38f, 1.0f);
        tint.z = RenderLightClamp(camera->skyHorizonColor.z / tintMaximum,
                                  0.38f, 1.0f);
    } else {
        tint = (Vec3){0.72f, 0.78f, 1.0f};
    }
    light->ambientColor.x = (0.18f + daylight * 0.20f) *
                            (0.65f + tint.x * 0.35f);
    light->ambientColor.y = (0.18f + daylight * 0.20f) *
                            (0.65f + tint.y * 0.35f);
    light->ambientColor.z = (0.18f + daylight * 0.20f) *
                            (0.65f + tint.z * 0.35f);
    light->diffuseColor.x = (0.52f + daylight * 0.34f) * tint.x;
    light->diffuseColor.y = (0.52f + daylight * 0.34f) * tint.y;
    light->diffuseColor.z = (0.52f + daylight * 0.34f) * tint.z;
}

int32_t RenderClampCarToGround(int32_t carY, int32_t groundY) {
    /* Game Y grows downwards.  Smaller values are valid airborne positions;
     * larger values would put the authored tyre/underbody contact plane below
     * the track surface retained in modelY. */
    return carY > groundY ? groundY : carY;
}

void RenderWorldInit(RenderWorld *world,
                         RenderMeshInstance *instances,
                         uint32_t capacity) {
    if (world == NULL) return;
    memset(world, 0, sizeof(*world));
    world->instances = instances;
    world->instanceCapacity = capacity;
    RenderDirectionalLightDefault(&world->light);
}

void RenderWorldBeginFrame(RenderWorld *world, uint64_t frame) {
    if (world == NULL) return;
    if (world->hasCamera) world->previousCamera = world->camera;
    if (world->hasMirrorCamera) {
        world->previousMirrorCamera = world->mirrorCamera;
        world->previousMirrorPanelY = world->mirrorPanelY;
    }
    world->frame = frame;
    world->instanceCount = 0;
    world->overflowCount = 0;
    world->spotLightCount = 0;
}

int RenderWorldSubmitSpotLight(RenderWorld *world, const SpotLight *light) {
    float length;
    if (!world || !light || world->spotLightCount >= RENDER_SPOT_LIGHT_CAPACITY)
        return 0;
    if (!isfinite(light->position.x) || !isfinite(light->position.y) ||
        !isfinite(light->position.z) || !isfinite(light->range) ||
        light->range <= 0 || !isfinite(light->color.x) ||
        !isfinite(light->color.y) || !isfinite(light->color.z) ||
        light->color.x < 0 || light->color.y < 0 || light->color.z < 0 ||
        !isfinite(light->innerCos) || !isfinite(light->outerCos) ||
        light->outerCos < -1 || light->innerCos > 1 ||
        light->innerCos <= light->outerCos) return 0;
    length = sqrtf(light->direction.x * light->direction.x +
                   light->direction.y * light->direction.y +
                   light->direction.z * light->direction.z);
    if (!isfinite(length) || length <= 0.000001f) return 0;
    SpotLight *out = &world->spotLights[world->spotLightCount++];
    *out = *light;
    out->direction.x /= length;
    out->direction.y /= length;
    out->direction.z /= length;
    return 1;
}

void RenderWorldSetDirectionalLight(
    RenderWorld *world, const RenderDirectionalLight *light) {
    if (world == NULL || light == NULL) return;
    world->light = *light;
}

void RenderWorldSetCamera(RenderWorld *world,
                              const RenderCamera *camera) {
    if (world == NULL || camera == NULL) return;
    if (world->hasCamera &&
        RenderCameraIsCut(&world->previousCamera, camera))
        world->previousCamera = *camera;
    world->camera = *camera;
    if (!world->hasCamera) world->previousCamera = *camera;
    world->hasCamera = 1;
}

void RenderWorldSetMirrorCamera(RenderWorld *world,
                                    const RenderCamera *camera,
                                    int active, float panelY) {
    if (world == NULL || camera == NULL) return;
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

int RenderWorldSubmitMesh(RenderWorld *world,
                          const RenderMeshInstance *instance) {
    if (world == NULL || instance == NULL) return 0;
    if (world->instances == NULL ||
        world->instanceCount >= world->instanceCapacity) {
        world->overflowCount++;
        return 0;
    }
    world->instances[world->instanceCount++] = *instance;
    return 1;
}

void RenderWorldDiscardPass(RenderWorld *world, RenderPass pass) {
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
                                    RenderTransform *transform) {
    if (transform == NULL) return;
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
    if (source == NULL || out == NULL) return;
    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            out[row][column] = source[row][column] * sign[row] * sign[column];
        }
    }
}
