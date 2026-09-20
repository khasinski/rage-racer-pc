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
    return failures != 0;
}
