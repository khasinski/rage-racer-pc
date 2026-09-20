#include "render/car_lights.h"
#include "render/car_lamps.h"

#include <math.h>
#include <stdio.h>

static int failures;
#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); \
    failures++; } } while (0)

int main(void) {
    CarLights car = {0}, rival = {0};
    UpdateCarLights(&car, 1, 1, 0, 1);
    CHECK(car.headlights == 0 && car.tail == 0 && car.stop == 0);
    UpdateCarLights(&car, 1, 1, 1, 0.02f);
    CHECK(car.stop == 1 && car.headlights == 0);
    UpdateCarLights(&car, 1, 0.25f, 0, 0.1f);
    CHECK(car.automatic && fabsf(car.headlights - 0.5f) < 0.0001f);
    CHECK(car.tail > 0 && car.stop == 0);
    UpdateCarLights(&car, 1, 0.32f, 0, 1);
    CHECK(car.automatic && car.headlights == 1);
    UpdateCarLights(&car, 1, 1, 0, 1);
    CHECK(!car.automatic && car.headlights == 0);
    UpdateCarLights(&car, 1, 0.32f, 0, 1);
    CHECK(!car.automatic && car.headlights == 0);
    UpdateCarLights(&car, 0.1f, 1, 1, 1);
    CHECK(car.headlights == 1 && car.stop == 1 && car.tail < car.stop);
    UpdateCarLights(&rival, 0.1f, 1, 1, 1);
    CHECK(rival.headlights == car.headlights && rival.stop == car.stop);
    UpdateCarLights(&car, 1, 1, 0, 0);
    CHECK(car.headlights == 1 && car.stop == 1);
    UpdateCarLights(&car, 1, 1, 0, NAN);
    CHECK(car.headlights == 1 && car.stop == 1);
    UpdateCarLights(&car, 1, 1, 0, -1);
    CHECK(car.headlights == 1 && car.stop == 1);
    /* One second produces the same settled state at PAL/NTSC tick rates. */
    car = (CarLights){0};
    rival = (CarLights){0};
    for (int i = 0; i < 50; i++) UpdateCarLights(&car, 0, 1, 0, 1.0f/50);
    for (int i = 0; i < 60; i++) UpdateCarLights(&rival, 0, 1, 0, 1.0f/60);
    CHECK(car.headlights == 1 && rival.headlights == 1);
    CHECK(CarLampIntensity(&(CarLights){0, 0.2f, 1, 0}, LAMP_TAIL) == 0.2f);
    CHECK(CarLampIntensity(&(CarLights){0, 0.2f, 1, 0}, LAMP_STOP) == 1);
    CHECK(CarLampIntensity(&(CarLights){0, 0.2f, 1, 0}, LAMP_TAIL_STOP) == 1);
    RenderMeshInstance body = {0};
    body.assetSet = RAGE_RENDER_ASSET_MODEL_BANK;
    body.assetKey = 68;
    body.lamps.headlights = 1;
    body.transform.scale = (Vec3){0.25f, 0.25f, 0.25f};
    body.transform.position = (Vec3){10, 20, 30};
    RenderWorld world;
    RenderWorldInit(&world, &body, 1);
    world.instanceCount = 1;
    RenderCarSpotLights(&world);
    CHECK(world.spotLightCount == 2);
    CHECK(fabsf(world.spotLights[0].position.x - (10 + 91.88095f * 0.25f)) < 0.001f);
    CHECK(fabsf(world.spotLights[0].position.z - (30 + 434.88889f * 0.25f)) < 0.001f);
    CHECK(world.spotLights[0].direction.z > 0.99f);
    CHECK(world.spotLights[0].direction.y < 0);
    world.spotLightCount = 0;
    body.transform.hasOrientation = 1;
    body.transform.orientation = (Quaternion){0, 1, 0, 0};
    RenderCarSpotLights(&world);
    CHECK(world.spotLights[0].direction.z < -0.99f);
    CHECK(fabsf(world.spotLights[0].position.z - (30 - 434.88889f * 0.25f)) < 0.001f);
    world.spotLightCount = 0;
    body.lamps.headlights = 0;
    RenderCarSpotLights(&world);
    CHECK(world.spotLightCount == 0);
    body.lamps.headlights = 1;
    body.lamps.tail = 0.2f;
    body.lamps.stop = 1;
    RenderCarSpotLights(&world);
    CHECK(world.spotLightCount == 4);
    CHECK(world.spotLights[2].direction.z > 0.99f); /* Rotated rear faces away from front. */
    CHECK(world.spotLights[2].color.x == 1.5f);
    CHECK(world.spotLights[2].position.x < body.transform.position.x);
    CHECK(world.spotLights[3].position.x > body.transform.position.x);
    world.spotLightCount = 0;
    body.pass = RAGE_RENDER_PASS_MIRROR;
    RenderCarSpotLights(&world);
    CHECK(world.spotLightCount == 0);
    body.pass = RAGE_RENDER_PASS_MAIN;
    body.assetSet = RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1;
    body.assetKey = 128;
    body.transform.hasOrientation = 0;
    body.lamps = (CarLights){0, 0.2f, 0, 1};
    RenderCarSpotLights(&world);
    CHECK(world.spotLightCount == 2);
    CHECK(world.spotLights[0].position.x < body.transform.position.x);
    CHECK(world.spotLights[1].position.x > body.transform.position.x);
    CHECK(world.spotLights[0].direction.z < -0.99f);
    CHECK(world.spotLights[1].direction.z < -0.99f);
    float tail = world.spotLights[0].color.x;
    world.spotLightCount = 0;
    body.lamps.stop = 1;
    RenderCarSpotLights(&world);
    CHECK(world.spotLightCount == 2);
    CHECK(fabsf(world.spotLights[0].color.x - 5 * tail) < 0.0001f);
    CHECK(world.spotLights[0].color.x == world.spotLights[1].color.x);
    body.assetSet = RAGE_RENDER_ASSET_MODEL_BANK;
    body.assetKey = 24;
    body.lamps = (CarLights){1, 0.2f, 0, 1};
    world.spotLightCount = 0;
    RenderCarSpotLights(&world);
    CHECK(world.spotLightCount == 4);
    for (int i = 0; i < 4; ++i) {
        CHECK(i < 2 ? world.spotLights[i].direction.z > 0.99f
                    : world.spotLights[i].direction.z < -0.99f);
        CHECK(i < 2 ? world.spotLights[i].position.z > body.transform.position.z
                    : world.spotLights[i].position.z < body.transform.position.z);
    }
    body.lamps = (CarLights){0, 0, 1, 0};
    world.spotLightCount = 0;
    RenderCarSpotLights(&world);
    CHECK(world.spotLightCount == 2); /* Braking during the day lights only the rear. */
    CHECK(world.spotLights[0].direction.z < -0.99f);
    CHECK(world.spotLights[1].direction.z < -0.99f);
    /* Every mapped player body has a pair at each end. Validate dark/off and
     * day/braking separately so adding a model cannot silently omit one end. */
    const unsigned models[] = {10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36,
                               46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72};
    body.assetSet = RAGE_RENDER_ASSET_MODEL_BANK;
    for (unsigned m = 0; m < sizeof(models) / sizeof(*models); ++m) {
        body.assetKey = models[m];
        const Lamp *lamps;
        unsigned count = CarLamps(&body, &lamps);
        CHECK(count == 4);
        for (unsigned i = 0; i < count; ++i) {
            CHECK(lamps[i].bounds[0] < lamps[i].bounds[2]);
            CHECK(lamps[i].bounds[1] < lamps[i].bounds[3]);
            CHECK(isfinite(lamps[i].position.x) && isfinite(lamps[i].position.y));
            CHECK(i < 2 ? lamps[i].position.z > 0 : lamps[i].position.z < 0);
        }
        body.lamps = (CarLights){1, 0.2f, 0, 1};
        world.spotLightCount = 0;
        RenderCarSpotLights(&world);
        CHECK(world.spotLightCount == 4);
        body.lamps = (CarLights){0, 0, 1, 0};
        world.spotLightCount = 0;
        RenderCarSpotLights(&world);
        CHECK(world.spotLightCount == 2);
        body.lamps = (CarLights){0};
        world.spotLightCount = 0;
        RenderCarSpotLights(&world);
        CHECK(world.spotLightCount == 0);
        body.component = 1;
        CHECK(CarLamps(&body, &lamps) == 0 && lamps == NULL);
        body.component = 0;
        body.mesh = 1;
        CHECK(CarLamps(&body, &lamps) == 0 && lamps == NULL);
        body.mesh = 0;
    }
    RenderMeshInstance grid[12] = {0};
    for (unsigned key = 38; key <= 44; key += 2) {
        if (key == 42) continue; /* This variant has blanked-off headlight panels. */
        body.assetKey = key;
        body.lamps = (CarLights){1, 0.2f, 1, 1};
        world.spotLightCount = 0;
        RenderCarSpotLights(&world);
        CHECK(world.spotLightCount == 6);
        body.lamps = (CarLights){0, 0, 1, 0};
        world.spotLightCount = 0;
        RenderCarSpotLights(&world);
        CHECK(world.spotLightCount == 4);
    }
    RenderWorldInit(&world, grid, 12);
    world.instanceCount = 12;
    for (int i = 0; i < 12; ++i) {
        grid[i].assetSet = RAGE_RENDER_ASSET_MODEL_BANK;
        grid[i].assetKey = 38;
        grid[i].entity = i;
        grid[i].transform.scale = (Vec3){1, 1, 1};
        grid[i].transform.position.x = i * 1000;
        grid[i].lamps = (CarLights){1, 0.2f, 1, 1};
    }
    RenderCarSpotLights(&world);
    CHECK(world.spotLightCount == 72);
    CHECK(world.spotLights[71].position.x > 10000);
    return failures != 0;
}
