#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "render/render_projection.h"

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)
static uint32_t state = 0x374986u;
static float random_float(void) {
    state ^= state << 13; state ^= state >> 17; state ^= state << 5;
    return ((int)(state % 200001) - 100000) / 100.0f;
}
static int same(float a, float b) { return a == b || (isnan(a) && isnan(b)); }
int main(void) {
    unsigned count = 0;
    for (int iteration = 0; iteration < 4000; ++iteration) {
        RageRenderCamera camera = {0};
        camera.transform.position = (RageRenderVec3){
            random_float(), random_float(), random_float()};
        camera.transform.rotation = (RageRenderVec3){
            random_float(), random_float(), random_float()};
        camera.transform.hasOrientation = iteration % 2;
        camera.transform.orientation = (RageRenderQuaternion){
            random_float(), random_float(), random_float(), random_float()};
        if (iteration % 11 == 0)
            camera.transform.orientation = (RageRenderQuaternion){0};
        if (iteration % 13 == 0) camera.transform.orientation.x = NAN;
        if (iteration % 17 == 0) camera.transform.orientation.w = INFINITY;
        if (iteration % 19 == 0)
            camera.transform.orientation = (RageRenderQuaternion){1e-30f, 0, 0, 1e-30f};
        if (iteration % 23 == 0) camera.transform.rotation.y = NAN;
        camera.fogNear = 1.0f + fabsf(random_float());
        camera.fogFar = camera.fogNear + 1.0f + fabsf(random_float());
        if (iteration % 7 == 0) camera.fogFar = camera.fogNear;
        if (iteration % 29 == 0) camera.fogNear = NAN;
        RageRenderViewTransform prepared = RenderPrepareView(&camera);
        for (int point = 0; point < 64; ++point) {
            RageRenderVec3 world = {random_float(), random_float(), random_float()};
            RageRenderVec3 expected, actual;
            if (point == 0) world.x = NAN;
            if (point == 1) world.z = INFINITY;
            RenderWorldToView(&camera, &world, &expected);
            RenderWorldToViewPrepared(&prepared, &world, &actual);
            CHECK(same(expected.x, actual.x));
            CHECK(same(expected.y, actual.y));
            CHECK(same(expected.z, actual.z));
            CHECK(same(RenderFogFactor(&camera, &world),
                       RenderFogFactorPrepared(&prepared, &world)));
            ++count;
        }
        /* Prepared state must survive mutation/reuse of a source world. */
        RageRenderCamera saved = camera;
        camera.transform.position.x += 500;
        RageRenderVec3 world = {12, 25, -45}, a, b;
        RenderWorldToView(&saved, &world, &a);
        RenderWorldToViewPrepared(&prepared, &world, &b);
        CHECK(same(a.x, b.x) && same(a.y, b.y) && same(a.z, b.z));
    }
    RageRenderViewTransform absent = RenderPrepareView(NULL);
    RageRenderVec3 world = {1, 2, 3}, view = world;
    RenderWorldToViewPrepared(&absent, &world, &view);
    CHECK(view.x == 0 && view.y == 0 && view.z == 0);
    CHECK(RenderFogFactorPrepared(&absent, &world) == 0);
    RenderWorldToViewPrepared(NULL, &world, &view);
    CHECK(view.x == 0 && view.y == 0 && view.z == 0);
    RenderWorldToViewPrepared(&absent, NULL, &view);
    RenderWorldToViewPrepared(&absent, &world, NULL);
    CHECK(RenderFogFactorPrepared(NULL, &world) == 0);
    printf("Prepared view: %u exact reference transforms and fog values matched\n", count);
    return 0;
}
