#include "car_parts.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
#define NEAR(a, b) (fabsf((a) - (b)) < 0.001f)

int main(void) {
    GameCarRuntime car = {0};
    car.x = 100;
    car.y = 150;
    car.modelY = 100;
    car.z = 300;
    car.bodyYaw = ANGLE_HALF_TURN;
    CarShape shape = {30, -20, 80, 10};
    CarPart first[CAR_PART_COUNT], second[CAR_PART_COUNT];
    const GameCarRuntime saved = car;
    CHECK(BuildCarParts(&car, &shape, 0, first));
    CHECK(memcmp(&car, &saved, sizeof(car)) == 0);
    CHECK(NEAR(first[0].position.y, 90));
    CHECK(NEAR(first[1].position.y, 90));
    CHECK(NEAR(first[2].position.x, 130) && NEAR(first[3].position.x, 70));
    CHECK(NEAR(first[2].position.y, 70) && NEAR(first[2].position.z, 380));
    CHECK(NEAR(first[2].rotation.m[0][0], 1));
    CHECK(NEAR(first[3].rotation.m[0][0], -1));

    GameCarRuntime other = car;
    other.x = 500;
    CarShape otherShape = {40, -10, 50, 20};
    CHECK(BuildCarParts(&other, &otherShape, ANGLE_QUARTER_TURN, second));
    CHECK(NEAR(second[0].position.x, 500) && NEAR(second[0].position.y, 80));
    CHECK(NEAR(second[2].position.x, 540) && NEAR(second[2].position.z, 350));
    CHECK(NEAR(second[2].rotation.m[0][2], -1));
    CHECK(NEAR(second[3].rotation.m[0][2], 1));
    CHECK(NEAR(first[2].position.x, 130));

RenderMeshInstance prototype = {.assetKey = 88, .assetSet = RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1,
    .assetSource = RENDER_ASSET_OWNED, .entity = 7, .mesh = 25, .materialVariant = 8,
    .lamps = {.headlights = 0.5f, .stop = 1}};
RenderMeshInstance instances[CAR_PART_COUNT], repeated[CAR_PART_COUNT];
CHECK(BuildCarInstances(&other, &car, &otherShape, &prototype, 28, 27, instances));
CHECK(instances[0].entity == 7 && instances[0].assetKey == 88);
CHECK(instances[0].assetSource == RENDER_ASSET_OWNED && instances[0].mesh == 25);
CHECK(NEAR(instances[0].transform.position.x, 500));
CHECK(NEAR(instances[0].previousTransform.position.x, 100));
CHECK(NEAR(instances[0].transform.position.y, -80));
CHECK(NEAR(instances[0].transform.scale.x, 0.25f));
CHECK(instances[0].lamps.stop == 1 && instances[1].lamps.stop == 0);
CHECK(instances[1].mesh == 28 && instances[2].mesh == 27 && instances[3].mesh == 27);
CHECK(instances[1].component == 2 && instances[2].component == 3 && instances[3].component == 4);
CHECK(instances[0].materialVariant == 8 && instances[1].materialVariant == 6);
CHECK(BuildCarInstances(&other, &car, &otherShape, &prototype, 28, 27, repeated));
CHECK(memcmp(instances, repeated, sizeof(instances)) == 0);
CHECK(!BuildCarInstances(NULL, &car, &shape, &prototype, 28, 27, repeated));
CHECK(!BuildCarInstances(&other, NULL, &shape, &prototype, 28, 27, repeated));
CHECK(memcmp(instances, repeated, sizeof(instances)) == 0);
prototype.assetSet = RAGE_RENDER_ASSET_COURSE;
CHECK(!BuildCarInstances(&other, &car, &shape, &prototype, 28, 27, repeated));
CHECK(memcmp(instances, repeated, sizeof(instances)) == 0);

    car.bodyRoll = ANGLE_QUARTER_TURN;
    CHECK(BuildCarParts(&car, &shape, 0, second));
    CHECK(NEAR(second[2].position.x, 120) && NEAR(second[3].position.x, 120));
    CHECK(NEAR(second[2].position.y, 120) && NEAR(second[3].position.y, 60));
    car.bodyRollVelocity = ANGLE_QUARTER_TURN;
    CHECK(BuildCarParts(&car, &shape, 0, second));
    CHECK(NEAR(second[2].position.x, 130) && NEAR(second[3].position.x, 70));

    memcpy(first, second, sizeof(first));
    CHECK(!BuildCarParts(NULL, &shape, 0, second));
    CHECK(!BuildCarParts(&car, NULL, 0, second));
    CHECK(!BuildCarParts(&car, &shape, 0, NULL));
    CHECK(memcmp(first, second, sizeof(first)) == 0);
    car.bodyYaw = INT_MIN;
    car.bodyRoll = INT_MIN;
    car.bodyRollVelocity = INT_MAX;
    car.y = car.modelY = INT_MIN;
    CHECK(BuildCarParts(&car, &shape, INT_MAX, second));
    for (unsigned i = 0; i < CAR_PART_COUNT; i++) {
        CHECK(isfinite(second[i].position.y));
        for (unsigned row = 0; row < 3; row++)
            for (unsigned column = 0; column < 3; column++)
                CHECK(isfinite(second[i].rotation.m[row][column]));
    }
    return 0;
}
