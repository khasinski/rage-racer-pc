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
static int close_enough(float a, float b) {
    float scale = fmaxf(1.0f, fmaxf(fabsf(a), fabsf(b)));
    return fabsf(a - b) <= scale * 2e-4f;
}
int main(void) {
    unsigned count = 0;
    for (int iteration = 0; iteration < 4000; ++iteration) {
        RenderCamera camera = {0};
        camera.transform.position = (Vec3){
            random_float(), random_float(), random_float()};
        camera.transform.rotation = (Vec3){
            random_float(), random_float(), random_float()};
        camera.transform.hasOrientation = iteration % 2;
        camera.transform.orientation = (Quaternion){
            random_float(), random_float(), random_float(), random_float()};
        if (iteration % 11 == 0)
            camera.transform.orientation = (Quaternion){0};
        if (iteration % 13 == 0) camera.transform.orientation.x = NAN;
        if (iteration % 17 == 0) camera.transform.orientation.w = INFINITY;
        if (iteration % 19 == 0)
            camera.transform.orientation = (Quaternion){1e-30f, 0, 0, 1e-30f};
        if (iteration % 23 == 0) camera.transform.rotation.y = NAN;
        camera.fogNear = 1.0f + fabsf(random_float());
        camera.fogFar = camera.fogNear + 1.0f + fabsf(random_float());
        if (iteration % 7 == 0) camera.fogFar = camera.fogNear;
        if (iteration % 29 == 0) camera.fogNear = NAN;
        RenderViewTransform prepared = RenderPrepareView(&camera);
        for (int point = 0; point < 64; ++point) {
            Vec3 world = {random_float(), random_float(), random_float()};
            Vec3 expected, actual;
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
        RenderCamera saved = camera;
        camera.transform.position.x += 500;
        Vec3 world = {12, 25, -45}, a, b;
        RenderWorldToView(&saved, &world, &a);
        RenderWorldToViewPrepared(&prepared, &world, &b);
        CHECK(same(a.x, b.x) && same(a.y, b.y) && same(a.z, b.z));
    }
    RenderViewTransform absent = RenderPrepareView(NULL);
    Vec3 world = {1, 2, 3}, view = world;
    RenderWorldToViewPrepared(&absent, &world, &view);
    CHECK(view.x == 0 && view.y == 0 && view.z == 0);
    CHECK(RenderFogFactorPrepared(&absent, &world) == 0);
    RenderWorldToViewPrepared(NULL, &world, &view);
    CHECK(view.x == 0 && view.y == 0 && view.z == 0);
    RenderWorldToViewPrepared(&absent, NULL, &view);
    RenderWorldToViewPrepared(&absent, &world, NULL);
    CHECK(RenderFogFactorPrepared(NULL, &world) == 0);

    for (int useQuaternion = 0; useQuaternion < 2; ++useQuaternion) {
        RenderCamera camera = {0};
        camera.transform.position = (Vec3){31.0f, -12.0f, 77.0f};
        camera.transform.rotation = (Vec3){-17.0f, 38.0f, 4.0f};
        camera.transform.hasOrientation = useQuaternion;
        camera.transform.orientation = (Quaternion){
            0.125f, -0.25f, 0.375f, 0.875f};
        camera.verticalFovDegrees = 55.0f;
        camera.nearPlane = 0.5f;
        camera.farPlane = 5000.0f;
        for (int point = 0; point < 100; ++point) {
            Vec3 viewPoint = {
                random_float() * 0.1f,
                random_float() * 0.1f,
                -1.0f - fabsf(random_float())};
            Vec3 clip, reconstructed, worldPoint;
            CHECK(RenderProject(&camera, &viewPoint, 16.0f / 9.0f, &clip));
            CHECK(RenderUnproject(&camera, 16.0f / 9.0f, &clip,
                                  &worldPoint));
            RenderWorldToView(&camera, &worldPoint, &reconstructed);
            CHECK(close_enough(reconstructed.x, viewPoint.x));
            CHECK(close_enough(reconstructed.y, viewPoint.y));
            CHECK(close_enough(reconstructed.z, viewPoint.z));
        }
    }
    Vec3 invalidClip = {0.0f, 0.0f, NAN};
    CHECK(!RenderUnproject(NULL, 1.0f, &invalidClip, &world));
    printf("Prepared view: %u exact reference transforms and fog values matched\n", count);
    return 0;
}
