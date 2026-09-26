#include "environment_view.h"
#include <stdio.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    Environment first = {.fogNear = 6000, .skyRowBase = 2};
    Environment second = {.fogNear = 32767, .skyRowBase = 1};
    for (unsigned slot = 0; slot < 9; ++slot) {
        first.colors.fields.slots[slot].cur.bytes.r = 255;
        second.colors.fields.slots[slot].cur.bytes.b = 255;
    }
    Environment saved = first;
    RenderCamera camera = {.verticalFovDegrees = 70, .nearPlane = 1, .farPlane = 16384,
                            .skyAssetKey = 88, .hasSkyLayout = 1};
    camera.transform.position = (Vec3){10, 20, 30};
    RenderCamera other = camera;
    ApplyEnvironment(&camera, &first);
    CHECK(camera.fogNear == 1500 && camera.fogFar == 7500);
    CHECK(camera.fogColor.x == 1 && camera.fogColor.z == 0);
    CHECK(camera.skyTopColor.x == 1 && camera.skyColor.x == 1);
    CHECK(camera.skyHorizonColor.x == 1 && camera.skyBottomColor.x == 1);
    CHECK(camera.skyCloudRow == 2);
    CHECK(camera.verticalFovDegrees == 70 && camera.farPlane == 16384);
    CHECK(camera.transform.position.y == 20 && camera.skyAssetKey == 88 && camera.hasSkyLayout);
    RenderCamera savedCamera = camera;
    ApplyEnvironment(&other, &second);
    CHECK(other.fogNear == 8191.75f && other.skyCloudRow == 1);
    CHECK(other.skyTopColor.z == 1 && other.skyTopColor.x == 0);
    CHECK(memcmp(&savedCamera, &camera, sizeof(camera)) == 0);
    CHECK(memcmp(&saved, &first, sizeof(first)) == 0);
    ApplyEnvironment(&camera, NULL);
    ApplyEnvironment(NULL, &first);
    CHECK(memcmp(&savedCamera, &camera, sizeof(camera)) == 0);
    return 0;
}
